/**
 * \file system_binding.c        \brief System bindings
 */
 
#include "uhab.h"

TRACE_TAG(binding_system);
#if !ENABLE_TRACE_SYSTEM
#include "trace_undef.h"
#endif


typedef struct system_timer
{
   struct system_timer *next;
   
   struct uhab_item *item;
   os_timer_type type;
   time_t value;
   uint32_t nticks;

   struct
   {
      int day;       // Day (1-31)
      int mon;       // Month (1-12)
      int hour;      // Hour (0-23)
      int min;       // Minute (0-59)
      int sec;       // Second (0-59)
   
      uint8_t periodical;  // Periodical timer at defined hour or min or sec
      
   } setting;
   
} system_timer_t;


// Prototypes:
static void second_timer_cb(void *arg);

// Locals:
static osTimerId second_timer;
//static int factory_reset_counter = 0;

LIST(timers);
static osMutexId mutex_timers;


/** Initialize binding */
static int system_binding_init(void)
{ 
   list_init(timers); 
   
   if ((mutex_timers = osMutexCreate(NULL)) == NULL)
   {
      TRACE_ERROR("Create mutex timers");
      throw_exception(fail);
   }

   TRACE("Init"); 

   return 0;
   
fail:
   return -1;   
}

/** Deinitialize binding */
static int system_binding_deinit(void)
{
   TRACE("Deinit");
   return 0;
}

/** Start binding */
static int system_binding_start(void)
{
   const osTimerDef(SECOND_TIMER, second_timer_cb);
   
   // Start periodic second timer
   if ((second_timer = osTimerCreate(osTimer(SECOND_TIMER), osTimerPeriodic, NULL)) == NULL)
   {
      TRACE_ERROR("Create second timer failed");
      return -1;
   }
   
   if (osTimerStart(second_timer, 1000) != osOK)
   {
      TRACE_ERROR("Start second timer failed");
      return -1;
   }
   
   TRACE("Start");

   return 0;
}

/*
 Format casu timeru: * * * * * (wday day mon hour min sec)
      day = den v mesici (1-31)
      mon = mesic (1 - 12)
      hour = hodina (0-23)
      min = minuta (0 - 59)
*/
#define CFG_SYSTEM_TIMER_NUM_ARGS      5
static system_timer_t *system_timer_create(struct uhab_item *item, char *type, char *mask)
{
   int argc, undefcnt = 0;
   char *argv[CFG_SYSTEM_TIMER_NUM_ARGS];
   system_timer_t *timer = NULL;
   
   if ((timer = os_malloc(sizeof(system_timer_t))) == NULL)
   {
      TRACE_ERROR("Alloc timer failed");
      throw_exception(fail);
   }
   os_memset(timer, 0, sizeof(system_timer_t));
   timer->item = item;
   
   if (!strcasecmp(type, "periodic"))
      timer->type = osTimerPeriodic;
   else if (!strcasecmp(type, "once"))
      timer->type = osTimerOnce;
   else
   {
      TRACE_ERROR("Bad type of timer %s - %s", type, mask);
      throw_exception(fail);
   }
   
   if ((argc = split_line(mask, ' ', argv, CFG_SYSTEM_TIMER_NUM_ARGS)) != CFG_SYSTEM_TIMER_NUM_ARGS)
   {
      TRACE_ERROR("Bad number of timer args %s - %s", type, mask);
      throw_exception(fail);
   }
   
   // Get time setting  
   timer->setting.day = (strcmp(argv[0], "*") != 0) ? atoi(argv[0]) : -1;
   timer->setting.mon = (strcmp(argv[1], "*") != 0) ? atoi(argv[1]) - 1 : -1;
   timer->setting.hour = (strcmp(argv[2], "*") != 0) ? atoi(argv[2]) : -1;
   timer->setting.min = (strcmp(argv[3], "*") != 0) ? atoi(argv[3]) : -1;
   timer->setting.sec = (strcmp(argv[4], "*") != 0) ? atoi(argv[4]) : -1;
        
   if (timer->setting.day == -1)
      undefcnt++;
   if (timer->setting.mon == -1)
      undefcnt++;
   if (timer->setting.hour == -1)
      undefcnt++;
   if (timer->setting.min == -1)
      undefcnt++;
   if (timer->setting.sec == -1)
      undefcnt++;
        
   if (undefcnt == CFG_SYSTEM_TIMER_NUM_ARGS - 1)
      timer->setting.periodical = 1;

   item->binding.protocol_item = timer;

   TRACE("  P:%d  %d %d %d %d %d", timer->setting.periodical, timer->setting.day, timer->setting.mon, timer->setting.hour, timer->setting.min, timer->setting.sec);
        
   return timer;
   
fail:
   if (timer != NULL)
      os_free(timer);
   
   return NULL;
}

static int system_timer_start(system_timer_t *timer)
{
   struct timeval curtime; 
   struct tm *tm;  
   
   gettimeofday(&curtime, NULL);
   
   if (timer->setting.periodical)
   {
      // Periodical time settings
      if (timer->setting.sec != -1)
         curtime.tv_sec += timer->setting.sec;

      if (timer->setting.min != -1)
         curtime.tv_sec += timer->setting.min * 60;

      if (timer->setting.hour != -1)
         curtime.tv_sec += timer->setting.hour * 3600;
         
      timer->value = curtime.tv_sec;
   }
   else
   {
      // Absolute time settings
      tm = localtime(&curtime.tv_sec);
      
      if (timer->setting.sec != -1)
         tm->tm_sec = timer->setting.sec;

      if (timer->setting.min != -1)
         tm->tm_min = timer->setting.min;

      if (timer->setting.hour != -1)
         tm->tm_hour = timer->setting.hour;      

      if (timer->setting.day != -1)
      {
         tm->tm_mday = timer->setting.day;
      }         
      else
      {
         // Execute at the next day
         tm->tm_mday++;
      }

      if (timer->setting.mon != -1)
      {
         tm->tm_mon = timer->setting.mon;
      }
      else
      {
         // Execute at the next month
         if (timer->setting.day != -1)
            tm->tm_mon++;
      }                 
      
      timer->value = mktime(tm);
   }
   
   //TRACE_PRINTFF("Start timer '%s' at %s", timer->item->name, ctime(&timer->value));
   
   return 0;
}

static int system_timer_stop(system_timer_t *timer)
{
   timer->value = 0;
   timer->nticks = 0;
   return 0;
}

/** Configure binding */
static int system_binding_configure(struct uhab_item *item, const char *binding_config)
{
   int ix;
   char *key, *value;
   char *params[CFG_BINDING_MAXNUM_ARGS];
   int params_count = CFG_BINDING_MAXNUM_ARGS;
   system_timer_t *timer = NULL;
     
   // Get first key/value with binding type
   if (uhab_config_parse_params((char *)binding_config, &key, &value, params, &params_count) != 0)
   {
      TRACE_ERROR("Parse binding params: '%s'", binding_config);
      throw_exception(fail);
   }   
   
   TRACE("   key: %s   value: %s  params_count: %d", key, value, params_count);
   for (ix = 0; ix < params_count; ix++)
   {
      TRACE("   param[%d] = %s", ix, params[ix]);
   }
   
   if (!strcasecmp(value, "timer"))
   {
      if (params_count != 2)
      {
         TRACE_ERROR("Bad number of timer params, required two params");
         throw_exception(fail);
      }
      
      // Parse and configure timer
      if ((timer = system_timer_create(item, params[0], params[1])) == NULL)
      {
         TRACE_ERROR("Configure system timer failed, %s - %s", params[0], params[1]);
         throw_exception(fail);
      }
            
      // Reload timer
      if (system_timer_start(timer) != 0)
      {
         TRACE_ERROR("Start system timer %s - %s", params[0], params[1]);
         throw_exception(fail);
      }
      
      osMutexWait(mutex_timers, osWaitForever);
      list_add(timers, timer);
      timer = NULL;
      osMutexRelease(mutex_timers);            
   }
   
   TRACE("Configure item: %s  config: %s", item->name, binding_config);        
   
   return 0;
   
fail:
   if (timer != NULL)
      os_free(timer);
   
   return -1;   
}

/** Send command (item state) to binded devices */
static int system_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   system_timer_t *timer;

   timer = item->binding.protocol_item;
   ASSERT(timer != NULL);

   osMutexWait(mutex_timers, osWaitForever);
   
   switch (state->type)
   {
      case UHAB_ITEM_STATE_TYPE_CMD:
      {  
         switch(state->value.cmd)
         {
            case UHAB_ITEM_STATE_CMD_ON:
            case UHAB_ITEM_STATE_CMD_START:
            {
               if (system_timer_start(timer) != 0)
                  throw_exception(fail);
            }
            break;
            
            case UHAB_ITEM_STATE_CMD_OFF:
            case UHAB_ITEM_STATE_CMD_STOP:
            {
               if (system_timer_stop(timer) != 0)
                  throw_exception(fail);
            }
            break;
            
            default:
               TRACE_ERROR("Not supported command: %d for item: %s", state->value.cmd, item->name);
               throw_exception(fail);
         }
      }
      break;
            
      default:
         TRACE_ERROR("Items state type: %d not suported, only command type", state->type);
         throw_exception(fail);
   }

   osMutexRelease(mutex_timers);

   uhab_item_state_t state_ticks = UHAB_ITEM_STATE_INIT_NUMBER(timer->nticks);

   // Update item state
   return uhab_bus_update(item, &state_ticks);

fail:
   osMutexRelease(mutex_timers);
   return -1;
}


static void second_timer_cb(void *arg)
{
   system_timer_t *timer;
   struct timeval curtime;

   hal_gpio_toggle(HAL_GPIO0);

/*   
#if defined(GPIO_BTN_LEFT) && defined (GPIO_BTN_RIGHT)
   if (!hal_gpio_get(GPIO_BTN_LEFT) && !hal_gpio_get(GPIO_BTN_RIGHT))
   {
      factory_reset_counter++;
      if (factory_reset_counter == 5)
      {
         system_factory_reset();
      }
   }
   else
   {      
      factory_reset_counter = 0;
   }
#endif   
*/
   gettimeofday(&curtime, NULL);
         
   osMutexWait(mutex_timers, osWaitForever);
   for (timer = list_head(timers); timer != NULL; timer = list_item_next(timer))
   {
      if (timer->value > 0 && curtime.tv_sec >= timer->value)
      {
         uhab_item_state_t state_ticks = UHAB_ITEM_STATE_INIT_NUMBER(++timer->nticks);
        
         VERIFY(uhab_bus_update(timer->item, &state_ticks) == 0);
         
         if (timer->type == osTimerPeriodic)
         {
            // Restart periodic timer
            VERIFY(system_timer_start(timer) == 0);
         }
         else
         {
            // Stop timer
            timer->value = 0;
         }
      }   
   }
   osMutexRelease(mutex_timers);
}


/** System binding interface definition */
uhab_protocol_binding_t system_binding = 
{
   .name = CFG_SYSTEM_BINDING_NAME,
   .label = "System",
   .init = system_binding_init,
   .deinit = system_binding_deinit,
   .start = system_binding_start,
   .configure = system_binding_configure,
   .send_command = system_binding_send
};
