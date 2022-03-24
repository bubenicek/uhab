/**
 * \file bus.c       \brief uHAB virtual BUS
 */

#include "uhab.h"

TRACE_TAG(bus);
#if !ENABLE_TRACE_BUS
#include "trace_undef.h"
#endif


// Prototypes:
static uhab_bus_waitstate_t *alloc_waitstate(uhab_sitemap_widget_t *parent_widget);
static void free_waitstate(uhab_bus_waitstate_t *ws);
static void contact_timer_cb(void *arg);
static void bus_thread(void *arg);

// Locals:
static const osThreadDef(BUS, bus_thread, CFG_BUS_THREAD_PRIORITY, 0, CFG_BUS_THREAD_STACK_SIZE);
static osThreadId thread;
static const osTimerDef(CONTACT_TIMER, contact_timer_cb);

static osPoolDef(BUS, CFG_UHAB_BUS_QUEUE_SIZE, uhab_bus_event_t);
static osPoolId pool;

const osMessageQDef(BUS, CFG_UHAB_BUS_QUEUE_SIZE, uint32_t);
static osMessageQId queue;

LIST(waitstates);
static osMutexId waitstate_mutex;

static uint32_t click_timelen = 200;
static uint32_t longclick_timelen = 1000;
static uint32_t longpress_timelen = 500;
static uint32_t waitchanges_timeout = CFG_UHAB_UIPROVIDER_POOL_TIMEOUT;

/** Initialize event bus */
int uhab_bus_init(void)
{
   char value[255];

   list_init(waitstates);

   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, CFG_SYSTEM_CONFIG_KEY_BUS_CLICK_TMLEN, value, sizeof(value)) == 0)
      click_timelen = atoi(value);

   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, CFG_SYSTEM_CONFIG_KEY_BUS_LONGCLICK_TMLEN, value, sizeof(value)) == 0)
      longclick_timelen = atoi(value);

   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, CFG_SYSTEM_CONFIG_KEY_BUS_LONGPRESS_TMLEN, value, sizeof(value)) == 0)
      longpress_timelen = atoi(value);

   if (uhab_config_service_get_value(CFG_SYSTEM_BINDING_NAME, CFG_SYSTEM_CONFIG_KEY_BUS_WAITCHANGES_TIMEOUT, value, sizeof(value)) == 0)
      waitchanges_timeout = atoi(value);

   if ((waitstate_mutex = osMutexCreate(NULL)) == NULL)
   {
      TRACE_ERROR("Alloc waitstate mutex");
      throw_exception(fail_mutex);
   }

   if ((queue = osMessageCreate(osMessageQ(BUS), osThreadGetId())) == NULL)
   {
      TRACE_ERROR("Create queue");
      throw_exception(fail_queue);
   }

   if ((pool = osPoolCreate(osPool(BUS))) == NULL)
   {
      TRACE_ERROR("Create pool");
      throw_exception(fail_pool);
   }

   if ((thread = osThreadCreate(osThread(BUS), NULL)) == 0)
   {
      TRACE_ERROR("Start thread");
      throw_exception(fail_thread);
   }

   TRACE("BUS init");

   return 0;

fail_thread:
fail_pool:
fail_queue:
   osMutexDelete(waitstate_mutex);
fail_mutex:
   return -1;
}

/** Deinitialize event bus */
int uhab_bus_deinit(void)
{
   VERIFY(osMutexDelete(waitstate_mutex) == osOK);
   return 0;
}

/** Internal send to one item with state transformation */
static int _uhab_bus_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   int res = 0;
   uhab_item_state_t newstate;
   const uhab_item_state_t *pstate = state;

   // Transform command state
   switch(item->state.type)
   {
      case UHAB_ITEM_STATE_TYPE_CMD:
      {
         if (state->value.cmd == UHAB_ITEM_STATE_CMD_TOGGLE)
         {
            // Toogle current state
            uhab_item_state_set_command(&newstate, item->state.value.cmd ^ 1);
            pstate = &newstate;
         }
      }
      break;

      default:
         break;
   }

   // Send or update item state
   if (item->binding.protocol != NULL)
   {
      ASSERT(item->binding.protocol->send_command != NULL);
      res += item->binding.protocol->send_command(item, pstate);
   }
   else
   {
      res += uhab_bus_update(item, pstate);
   }

   return res;
}

/** Send state to item and all child items */
int uhab_bus_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   int res = 0;
   uhab_child_item_t *child;

   ASSERT(item != NULL);
   ASSERT(state != NULL);

   if (item->type == UHAB_ITEM_TYPE_GROUP)
   {
      if (item->stereotype == UHAB_ITEM_STEREOTYPE_LIST)
      {
         res = _uhab_bus_send(item, state);
      }
      else
      {
         // Send command to all child items
         for (child = list_head(item->child_items); child != NULL; child = list_item_next(child))
         {
            res += _uhab_bus_send(child->item, state);
         }
      }
   }
   else
   {
      res = _uhab_bus_send(item, state);
   }

   return res;
}

/** Update binding item state */
int uhab_bus_update(const uhab_item_t *item, const uhab_item_state_t *state)
{
   uhab_bus_event_t *event;

   // Alloc event
   if ((event = osPoolAlloc(pool)) == NULL)
   {
      TRACE_ERROR("Alloc event");
      return -1;
   }
   os_memset(event, 0, sizeof(uhab_bus_event_t));

   // Set queued item state
   event->item = (uhab_item_t *)item;
   uhab_item_state_set(&event->state, state);

   // Add command to queue
   if (osMessagePut(queue, (uintptr_t)event, osWaitForever) != osOK)
   {
      TRACE_ERROR("Add event to queue");
      osPoolFree(pool, event);
      return -1;
   }

   return 0;
}

/** Wait for any changes */
int uhab_bus_waitfor_changes(uhab_sitemap_widget_t *parent_widget)
{
   uhab_bus_waitstate_t *ws;

   if ((ws = alloc_waitstate(parent_widget)) == NULL)
   {
      TRACE_ERROR("Alloc waitstate");
      return -1;
   }

   osSemaphoreWait(ws->sem, waitchanges_timeout);
   free_waitstate(ws);

   return 0;
}

/** Alloc waitstate */
static uhab_bus_waitstate_t *alloc_waitstate(uhab_sitemap_widget_t *parent_widget)
{
   uhab_bus_waitstate_t *ws = NULL;

   VERIFY(osMutexWait(waitstate_mutex, osWaitForever) == osOK);

   // Find allocated free waitstate
   for (ws = list_head(waitstates); ws != NULL; ws = list_item_next(ws))
   {
      if (!ws->active)
      {
         ws->active = 1;
         break;
      }
   }

   // Free waitstate does't found, alloc new waitstate
   if (ws == NULL)
   {
      if ((ws = os_malloc(sizeof(uhab_bus_waitstate_t))) == NULL)
      {
         TRACE_ERROR("Alloc new wait state");
         throw_exception(fail);
      }

      if ((ws->sem = osSemaphoreCreate(NULL, 1)) == NULL)
      {
         TRACE_ERROR("Create semaphore");
         os_free(ws);
         throw_exception(fail);
      }

      if (osSemaphoreWait(ws->sem, osWaitForever) != osOK)
      {
         TRACE_ERROR("Sem wait");
         osSemaphoreDelete(ws->sem);
         os_free(ws);
         throw_exception(fail);
      }

      ws->active = 1;
      list_add(waitstates, ws);

      TRACE("Alloc new waitstate");
   }

   ws->parent_widget = parent_widget;

   VERIFY(osMutexRelease(waitstate_mutex) == osOK);

   return ws;

fail:
   VERIFY(osMutexRelease(waitstate_mutex) == osOK);
   return NULL;
}

/** Free waitstate */
static void free_waitstate(uhab_bus_waitstate_t *ws)
{
   VERIFY(osMutexWait(waitstate_mutex, osWaitForever) == osOK);
   ws->active = 0;
   VERIFY(osMutexRelease(waitstate_mutex) == osOK);
}

/** Contact time length measurement timeout timer callback */
static void contact_timer_cb(void *arg)
{
   uhab_item_t *item = arg;
   uhab_rule_event_t rule_event;

   ASSERT(item != NULL);

   if (item->state.value.cmd == UHAB_ITEM_STATE_CMD_ON)
   {
      rule_event = UHAB_RULE_EVENT_LONGPRESS;
   }
   else
   {
      switch (item->bus.click.count)
      {
         case 1:
         {
            if (hal_time_ms() - item->bus.click.start_time >= longclick_timelen)
               rule_event = UHAB_RULE_EVENT_LONGCLICK;
            else
               rule_event = UHAB_RULE_EVENT_CLICK;
         }
         break;

         case 2:
            rule_event = UHAB_RULE_EVENT_DOUBLECLICK;
            break;

         case 3:
            rule_event = UHAB_RULE_EVENT_TRIPLECLICK;
            break;

         default:
            rule_event = UHAB_RULE_EVENT_QUADCLICK;
            break;
      }
   }

   // Process automatin rule event
   if (uhab_automation_process_event(&automation, rule_event, item, &item->state) != 0)
   {
      TRACE_ERROR("Automation process item: %s  event: %d", item->name, rule_event);
   }

   item->bus.click.count = 0;
   item->bus.click.start_time = 0;
}


/** Working thread */
static void bus_thread(void *arg)
{
   osEvent evt;
   uhab_bus_event_t *event;
   uhab_bus_waitstate_t *ws;
   uhab_rule_event_t rule_event;
#if ENABLE_TRACE_BUS_CHANGES
   char txt[255];
#endif

   TRACE("BUS thread is running ...");

   while(1)
   {
      // Wait for event in the queue
      evt = osMessageGet(queue, osWaitForever);
      if (evt.status != osEventMessage)
      {
         TRACE_ERROR("Get event from the queue");
         continue;
      }

      event = evt.value.p;

      // Translate item state to automation rule event type
      switch(event->state.type)
      {
         case UHAB_ITEM_STATE_TYPE_CMD:
         {
            switch(event->state.value.cmd)
            {
               case UHAB_ITEM_STATE_CMD_TOGGLE:
               {
                  // Toogle current state
                  event->state.value.cmd =  event->item->state.value.cmd ^ 1;
                  rule_event = (event->state.value.cmd) ? UHAB_RULE_EVENT_ON : UHAB_RULE_EVENT_OFF;
               }
               break;

               case UHAB_ITEM_STATE_CMD_ON:
               {
                  rule_event = UHAB_RULE_EVENT_ON;

                  if (event->item->type == UHAB_ITEM_TYPE_CONTACT)
                  {
                     // Click ON
                     event->item->bus.click.start_time = hal_time_ms();

                     // Stop click len timer
                     if (event->item->bus.click.timer == NULL)
                     {
                        // Create click length timer
                        if ((event->item->bus.click.timer = osTimerCreate(osTimer(CONTACT_TIMER), osTimerOnce, event->item)) == NULL)
                        {
                           TRACE_ERROR("Start contact: %s click timer", event->item->name);
                           break;
                        }
                     }

                     event->item->bus.click.count++;
                     VERIFY(osTimerStart(event->item->bus.click.timer, longpress_timelen) == osOK);
                  }
               }
               break;

               case UHAB_ITEM_STATE_CMD_OFF:
               {
                  rule_event = UHAB_RULE_EVENT_OFF;

                  if (event->item->type == UHAB_ITEM_TYPE_CONTACT)
                  {
                     // Click OFF
                     if (event->item->bus.click.timer == NULL)
                     {
                        // Create click length timer
                        if ((event->item->bus.click.timer = osTimerCreate(osTimer(CONTACT_TIMER), osTimerOnce, event->item)) == NULL)
                        {
                           TRACE_ERROR("Start contact: %s click timer", event->item->name);
                           break;
                        }
                     }

                     VERIFY(osTimerStart(event->item->bus.click.timer, click_timelen) == osOK);
                  }
               }
               break;

               case UHAB_ITEM_STATE_CMD_UP:
               {
                  rule_event = UHAB_RULE_EVENT_CHANGED;

                  if (event->item->stereotype == UHAB_ITEM_STEREOTYPE_LIST)
                  {
                     // Seek to next child item
                     if (event->item->active_child_item != NULL)
                     {
                        uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT_CMD(UHAB_ITEM_STATE_CMD_OFF);

                        // Deactivate child item
                        VERIFY(uhab_bus_send(event->item->active_child_item->item, &newstate) == 0);

                        event->item->active_child_item = list_item_next(event->item->active_child_item);
                        if (event->item->active_child_item == NULL)
                           event->item->active_child_item = list_head(event->item->child_items);

                        // Activete child_item
                        uhab_item_state_set_command(&newstate, UHAB_ITEM_STATE_CMD_ON);
                        VERIFY(uhab_bus_send(event->item->active_child_item->item, &newstate) == 0);
                     }
                  }
               }
               break;

               case UHAB_ITEM_STATE_CMD_DOWN:
               {
                  rule_event = UHAB_RULE_EVENT_CHANGED;

                  if (event->item->stereotype == UHAB_ITEM_STEREOTYPE_LIST)
                  {
                     if (event->item->active_child_item != NULL)
                     {
                        uhab_child_item_t *child_item, *prev_child;
                        uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT_CMD(UHAB_ITEM_STATE_CMD_OFF);

                        // Deactivate child item
                        VERIFY(uhab_bus_send(event->item->active_child_item->item, &newstate) == 0);

                        if (event->item->active_child_item == list_head(event->item->child_items))
                        {
                           // First child item, seek to end
                           event->item->active_child_item = list_tail(event->item->child_items);
                        }
                        else
                        {
                           // Seek to previous child item
                           for (prev_child = child_item = list_head(event->item->child_items); child_item != event->item->active_child_item; child_item = list_item_next(child_item))
                              prev_child = child_item;

                           event->item->active_child_item = prev_child;
                        }

                        // Activate child item
                        uhab_item_state_set_command(&newstate, UHAB_ITEM_STATE_CMD_ON);
                        VERIFY(uhab_bus_send(event->item->active_child_item->item, &newstate) == 0);
                     }
                  }
               }
               break;

               default:
                  rule_event = UHAB_RULE_EVENT_CHANGED;
                  break;
            }
         }
         break;

         case UHAB_ITEM_STATE_TYPE_NUMBER:
         {
            if (event->item->stereotype == UHAB_ITEM_STEREOTYPE_LIST)
            {
               if (event->item->active_child_item != NULL)
               {
                  int count = 0;
                  uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT_CMD(UHAB_ITEM_STATE_CMD_OFF);

                  // Deactivate active item
                  VERIFY(uhab_bus_send(event->item->active_child_item->item, &newstate) == 0);

                  // Seek to new active
                  for (count = 0, event->item->active_child_item = list_head(event->item->child_items);
                       event->item->active_child_item != NULL && count < event->state.value.number;
                       event->item->active_child_item = list_item_next(event->item->active_child_item), count++);

                  if (event->item->active_child_item != NULL)
                  {
                     // Activate child item
                     uhab_item_state_set_command(&newstate, UHAB_ITEM_STATE_CMD_ON);
                     VERIFY(uhab_bus_send(event->item->active_child_item->item, &newstate) == 0);
                  }
                  else
                  {
                     TRACE_ERROR("Select active child is out of range");
                  }
               }
            }

            rule_event = UHAB_RULE_EVENT_CHANGED;
         }

         default:
            rule_event = UHAB_RULE_EVENT_CHANGED;
            break;
      }

      // Process automatin rule event
      if (uhab_automation_process_event(&automation, rule_event, event->item, &event->state) != 0)
      {
         TRACE_ERROR("Automation process item: %s  event: %d", event->item->name, rule_event);
      }

      // Update item state
      uhab_item_state_set(&event->item->state, &event->state);

#if ENABLE_TRACE_BUS_CHANGES
      TRACE("Item: '%s' changed to: %s", event->item->name, uhab_item_state_get_value(&event->item->state, txt, sizeof(txt)));
#endif

      // Release all waiting states
      VERIFY(osMutexWait(waitstate_mutex, osWaitForever) == osOK);

      for (ws = list_head(waitstates); ws != NULL; ws = list_item_next(ws))
      {
         if (ws->active)
         {
            if (uhab_sitemap_find_item_widget(ws->parent_widget, event->item) != NULL)
               VERIFY(osSemaphoreRelease(ws->sem) == osOK);
         }
      }

      VERIFY(osMutexRelease(waitstate_mutex) == osOK);

      // Save update time
      event->item->bus.update_time = hal_time_ms();

      // Free bus event
      uhab_item_state_release(&event->state);
      osPoolFree(pool, event);
   }
}
