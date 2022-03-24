/**
 * \file dmx_binding.c        \brief DMX bindings
 */

#include <math.h>
#include "uhab.h"
#include "dmx_binding.h"

TRACE_TAG(binding_dmx);
#if !ENABLE_TRACE_DMX
#include "trace_undef.h"
#endif


// Prototypes:
static dmx_device_t *alloc_dmx_device(const char *name);
static int free_dmx_device(dmx_device_t *dev);
static int refresh_dmx_buffer(dmx_device_item_t *devitem);
static int dmx_fader_start(dmx_device_item_t *devitem, dmx_cmd_t *cmd, int periodical);
static dmx_cmd_t *dmx_fader_stop(dmx_device_item_t *devitem);
static int dmx_fader_is_running_down(dmx_device_item_t *devitem);
static void dmx_fader_timer_cb(void *arg);
static void dmx_poll_timer_cb(void *arg);
static void dmx_poll_thread(void *arg);

// Locals:
static uint8_t dmx_running = 0;
static const osThreadDef(DMX_POLL, dmx_poll_thread, CFG_DMX_THREAD_PRIORITY, 0, CFG_DMX_THREAD_STACK_SIZE);   
static osThreadId poll_thread;
static osSemaphoreId poll_sem;
static osTimerId poll_timer;
LIST(dmx_devices);
static int poll_interval = CFG_DMX_POLL_INTERVAL;
static int fadetick = CFG_DMX_FADETICK;
static int periodical_fadetime = CFG_DMX_PERIODICAL_FADETIME;
static int dmx_break_delay = CFG_DMX_MARK_AFTER_BREAK_DELAY;


/** Initialize binding */
static int dmx_binding_init(void)
{
   char value[255];
   list_init(dmx_devices);

   if (uhab_config_service_get_value(CFG_DMX_BINDING_NAME, "dmx.poll", value, sizeof(value)) == 0)
   {
      poll_interval = atoi(value);
      if (poll_interval < CFG_DMX_POLL_INTERVAL)
         poll_interval = CFG_DMX_POLL_INTERVAL;
   }

   if (uhab_config_service_get_value(CFG_DMX_BINDING_NAME, "dmx.fadetick", value, sizeof(value)) == 0)
   {
      fadetick = atoi(value);
      if (fadetick < CFG_DMX_FADETICK)
         fadetick = CFG_DMX_FADETICK;
   }

   if (uhab_config_service_get_value(CFG_DMX_BINDING_NAME, "dmx.periodical_fadetime", value, sizeof(value)) == 0)
   {
      periodical_fadetime = atoi(value);
      if (periodical_fadetime < CFG_DMX_FADETICK)
         periodical_fadetime = CFG_DMX_PERIODICAL_FADETIME;
   }

   if (uhab_config_service_get_value(CFG_DMX_BINDING_NAME, "dmx.break_delay", value, sizeof(value)) == 0)
   {
      dmx_break_delay = atoi(value);
   }

   TRACE("Init params:");
   TRACE("   poll = %d", poll_interval);
   TRACE("   fadetick = %d", fadetick);
   TRACE("   periodical_fadetime = %d", periodical_fadetime);
   TRACE("   dmx_break_delay = %d", dmx_break_delay);

   return 0;
}

/** Deinitialize binding */
static int dmx_binding_deinit(void)
{
   int res = 0;
   dmx_device_t *dev;

   // Remove all device items
   while((dev = list_head(dmx_devices)) != NULL)
   {
      list_remove(dmx_devices, dev);
      res += free_dmx_device(dev);
   }

   TRACE("Deinit");

   return res;
}

/** Start binding */
static int dmx_binding_start(void)
{
   const osTimerDef(DMX_TIMER, dmx_poll_timer_cb);
   dmx_device_t *dev;

   // Initialialize all devices
   for (dev = list_head(dmx_devices); dev != NULL; dev = list_item_next(dev))
   {
      if (hal_dmx_init(dev->dmx, dmx_break_delay) != 0)
      {
         TRACE_ERROR("Start DMX connection[%d]", dev->dmx);
         return -1;
      }

      TRACE("Start DMX connection[%d]  num channnels: %d", dev->dmx, dev->dmxbuf_length);
   }

   if (list_length(dmx_devices) > 0)
   {
      poll_sem = osSemaphoreCreate(NULL, 1);
      if (poll_sem == NULL)
      {
         TRACE_ERROR("Create semaphore");
         return -1;
      }
      osSemaphoreWait(poll_sem, osWaitForever);
      
      // Start poll thread
      if ((poll_thread = osThreadCreate(osThread(DMX_POLL), NULL)) == 0)
      {
         TRACE_ERROR("Start poll thread");
         return -1;
      }
      
      // Start periodic thread timer
      if ((poll_timer = osTimerCreate(osTimer(DMX_TIMER), osTimerPeriodic, dev)) == NULL)
      {
         TRACE_ERROR("Create timer failed");
         return -1;
      }

      if (osTimerStart(poll_timer, poll_interval) != osOK)
      {
         TRACE_ERROR("Start timer failed");
         return -1;
      }

      TRACE("Start DMX poll timer: %d  fadetick: %d  periodical_fadetime: %d", poll_interval, fadetick, periodical_fadetime);
   }

   dmx_running = 1;

   return 0;
}

/** Configure binding */
static int dmx_binding_configure(struct uhab_item *item, const char *binding_config)
{
   int argc, ix, iy;
   char *argv[CFG_DMX_MAXNUM_ARGS];
   char *key, *value;
   char *params[CFG_DMX_MAXNUM_ARGS];
   int params_count = CFG_DMX_MAXNUM_ARGS;
   dmx_device_t *dev = NULL;
   dmx_device_item_t *devitem = NULL;

   // Split to array
   argc = split_line((char *)binding_config, ' ', argv, CFG_DMX_MAXNUM_ARGS);
   if (argc < 3)
   {
      TRACE_ERROR("Bad number of args in binding config: %s", binding_config);
      throw_exception(fail);
   }

   // Get first key/value with binding type
   if (uhab_config_parse_params(argv[0], &key, &value, params, &params_count) != 0)
   {
      TRACE_ERROR("Parse DMX binding params: '%s'", argv[0]);
      throw_exception(fail);
   }

   // Alloe or use exists dmx device
   if ((dev = alloc_dmx_device(value)) == NULL)
   {
      TRACE_ERROR("Alloc device '%s'", value);
      throw_exception(fail);
   }

   // Alloc dmx device item
   if ((devitem = os_malloc(sizeof(dmx_device_item_t))) == NULL)
   {
      TRACE_ERROR("Alloc device item");
      throw_exception(fail);
   }

   os_memset(devitem, 0, sizeof(dmx_device_item_t));
   devitem->dev = dev;
   devitem->item = item;

   if ((devitem->fader.mutex = osMutexCreate(NULL)) == NULL)
   {
      TRACE_ERROR("Create fader mutex");
      throw_exception(fail);
   }

   list_add(dev->items, devitem);

   item->binding.protocol_item = devitem;

   // Set item used dmx channels
   for (ix = 0; ix < params_count; ix++)
   {
      if (ix == CFG_DMX_MAX_NUM_CHANNELS)
      {
         TRACE_ERROR("Max number of channels exceeded");
         throw_exception(fail);
      }

      devitem->channels[ix] = atoi(params[ix]);
      if (devitem->channels[ix] >= CFG_DMX_NUM_CHANNELS)
      {
         TRACE_ERROR("Channel number exceeed max. %d", CFG_DMX_NUM_CHANNELS);
         throw_exception(fail);
      }

      if (devitem->channels[ix] > dev->dmxbuf_length)
         dev->dmxbuf_length = devitem->channels[ix];

      devitem->channels_count++;
   }

   // Parse commands
   for (ix = 1; ix < argc; ix++)
   {
      if (uhab_config_parse_params(argv[ix], &key, &value, params, &params_count) == 0)
      {
         if (uhab_item_state_str2command(key, &devitem->commands[devitem->commands_count].cmdtype) != 0)
         {
            TRACE_ERROR("Not supported command type: %s", key);
            throw_exception(fail);
         }

         if (value != NULL)
         {
            devitem->commands[devitem->commands_count].fadetime = atoi(value);
            if (devitem->commands[devitem->commands_count].fadetime < fadetick)
               devitem->commands[devitem->commands_count].fadetime = fadetick;
         }

         if (params_count >= CFG_DMX_MAX_NUM_CHANNELS)
         {
            TRACE_ERROR("Max number of dmx channel values exceeded");
            throw_exception(fail);
         }

         TRACE_PRINTFF("  cmd: %s  fadetime: %d  channel_values: ", key, devitem->commands[devitem->commands_count].fadetime);
         for (iy = 0; iy < params_count; iy++)
         {
            TRACE_PRINTF("%s ", params[iy]);
            devitem->commands[devitem->commands_count].default_channel_values[iy] = devitem->commands[devitem->commands_count].channel_values[iy] = atoi(params[iy]);
         }
         TRACE_PRINTF("\n");
      }
      else
      {
         TRACE_ERROR("Parse error: %s", argv[ix]);
      }

      devitem->commands_count++;
   }

   TRACE("Configure item: %s  config: %s", item->name, binding_config);

   return 0;

fail:
   if (dev != NULL)
      free_dmx_device(dev);

   return -1;
}

/** Find DMX setings for command */
static dmx_cmd_t *find_dmx_command(dmx_device_item_t *devitem, uhab_item_state_cmd_t cmd)
{
   int ix;

   for (ix = 0; ix < devitem->commands_count; ix++)
   {
      if (devitem->commands[ix].cmdtype == cmd)
      {
         return &devitem->commands[ix];
      }
   }

   return NULL;
}

/** Send command (item state) to binded devices */
static int dmx_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   int ix;
   dmx_device_item_t *devitem;
   dmx_cmd_t *cmd = NULL;
   uhab_item_state_t newstate;

   if (!dmx_running)
   {
      TRACE_ERROR("DMX binding is not running");
      return -1;
   }

   devitem = item->binding.protocol_item;
   ASSERT(devitem != NULL);
   
   switch (state->type)
   {
      case UHAB_ITEM_STATE_TYPE_CMD:
      {
         switch(state->value.cmd)
         {
            case UHAB_ITEM_STATE_CMD_ON:
            case UHAB_ITEM_STATE_CMD_OFF:
            case UHAB_ITEM_STATE_CMD_TOGGLE:
            {
               if (state->value.cmd == UHAB_ITEM_STATE_CMD_TOGGLE)
               {
                  // Toggle current dimmer value
                  if (devitem->channels_values[0] > 0)
                  {
                     // If dimming is pending down then enable ON
                     if (dmx_fader_is_running_down(devitem))
                        cmd = find_dmx_command(devitem, UHAB_ITEM_STATE_CMD_ON);
                     else
                        cmd = find_dmx_command(devitem, UHAB_ITEM_STATE_CMD_OFF);
                  }
                  else
                  {
                     cmd = find_dmx_command(devitem, UHAB_ITEM_STATE_CMD_ON);
                     
                     // Set default ON when is zero
                     if (cmd != NULL && cmd->channel_values[0] == 0)
                        cmd->channel_values[0] = cmd->default_channel_values[0];
                  }
               }
               else
               {
                  cmd = find_dmx_command(devitem, state->value.cmd);
               }

               // Find command
               if (cmd == NULL)
               {
                  TRACE_ERROR("Not defined dmx command type: %d", state->value.cmd);
                  throw_exception(fail);
               }

               if (cmd->fadetime > 0)
               {
                  // Start oneshot fader
                  if (dmx_fader_start(devitem, cmd, 0) != 0)
                  {
                     TRACE_ERROR("DMX fader start failed");
                     throw_exception(fail);
                  }
               }
               else
               {
                  // Set channel values
                  for (ix = 0; ix < devitem->channels_count; ix++)
                     devitem->channels_values[ix] = cmd->channel_values[ix];

                  // Refresh DMX output buffer
                  refresh_dmx_buffer(devitem);
               }
            }
            break;
            
            // Start dimming 
            case UHAB_ITEM_STATE_CMD_START:
            {               
               if ((cmd = find_dmx_command(devitem, UHAB_ITEM_STATE_CMD_ON)) == NULL)
               {
                  TRACE_ERROR("Not defined ON command");
                  throw_exception(fail);
               }
               
               // Start periodical fader until stopped
               if (dmx_fader_start(devitem, cmd, 1) != 0)
               {
                  TRACE_ERROR("DMX fader start failed");
                  throw_exception(fail);
               }
            }
            break;

            // Stop running dimming 
            case UHAB_ITEM_STATE_CMD_STOP:
            {
               if (devitem->fader.periodical)
               {
                  if ((cmd = dmx_fader_stop(devitem)) == NULL)
                  {
                     TRACE_ERROR("DMX fader stop failed");
                     throw_exception(fail);
                  }
               }
            }
            break;

            default:
               TRACE_ERROR("Not supported command type: %d", state->value.cmd);
               throw_exception(fail);
         }
      }
      break;

      case UHAB_ITEM_STATE_TYPE_NUMBER:
      {
         int value;
         
         // Convert value from percent
         value = (CFG_DMX_MAX_CHANNEL_VALUE / 100.0) * state->value.number;

         // Find command by state value
         if ((cmd = find_dmx_command(devitem, (value > 0) ? UHAB_ITEM_STATE_CMD_ON : UHAB_ITEM_STATE_CMD_OFF)) == NULL)
         {
            TRACE_ERROR("Not defined dmx command ON");
            throw_exception(fail);
         }

         // Set command new value
         cmd->channel_values[0] = value;

         if (cmd->fadetime > 0)
         {
            // Start oneshot fader
            if (dmx_fader_start(devitem, cmd, 0) != 0)
            {
               TRACE_ERROR("DMX fader start failed");
               throw_exception(fail);
            }
         }
         else
         {
            // Set channel value immediately
            devitem->channels_values[0] = value;

            // Refresh DMX output buffer
            refresh_dmx_buffer(devitem);
         }
      }
      break;

      default:
         TRACE_ERROR("Items state type: %d not suported, only command type or number", state->type);
         throw_exception(fail);
   }
   
   if (cmd != NULL)
   {
      // Set new state in %
      uhab_item_state_set_number(&newstate, round(cmd->channel_values[0] / (CFG_DMX_MAX_CHANNEL_VALUE / 100.0)));

      // Update item state
      return uhab_bus_update(item, &newstate);
   }
   else
   {
      return 0;
   }

fail:
   return -1;
}

/** Alloc new or use existing dmx device */
static dmx_device_t *alloc_dmx_device(const char *name)
{
   char key[255];
   char value[255];
   dmx_device_t *dev = NULL;
   char *pp;

   // Try to find exists device
   for (dev = list_head(dmx_devices); dev != NULL; dev = list_item_next(dev))
   {
      if (!strcmp(dev->name, name))
         break;
   }

   // Device not exists, alloc new
   if (dev == NULL)
   {
      if ((dev = os_malloc(sizeof(dmx_device_t))) == NULL)
      {
         TRACE_ERROR("Alloc device name");
         throw_exception(fail_alloc_dev);
      }
      os_memset(dev, 0, sizeof(dmx_device_t));

      LIST_STRUCT_INIT(dev, items);
      dev->name = os_strdup(name);

      // Get connection
      snprintf(key, sizeof(key), "%s.connection", name);
      if (uhab_config_service_get_value(CFG_DMX_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);
         throw_exception(fail_key);
      }

      if ((pp = strchr(value, ':')) == NULL)
      {
         TRACE_ERROR("Bad connection format: %s", value);
         throw_exception(fail_key);
      }
      pp++;
      dev->dmx = atoi(pp);

      snprintf(key, sizeof(key), "%s.min_channels_count", name);
      if (uhab_config_service_get_value(CFG_DMX_BINDING_NAME, key, value, sizeof(value)) == 0)
      {
         dev->dmxbuf_length = atoi(value);
      }
      else
      {
         dev->dmxbuf_length = CFG_DMX_MIN_NUM_CHANNELS;
         TRACE("Using default min_channels_count: %d", dev->dmxbuf_length);
      }

      list_add(dmx_devices, dev);
      TRACE("Alloc DMX device name: %s  connection: %d  min_channels_count: %d", dev->name, dev->dmx, dev->dmxbuf_length);
   }

   return dev;

fail_key:
   free_dmx_device(dev);
fail_alloc_dev:
   return NULL;
}

/** Free dmx device allocated resources */
static int free_dmx_device(dmx_device_t *dev)
{
   dmx_device_item_t *devitem;

   ASSERT(dev != NULL);

   // Remove all device items
   while((devitem = list_head(dev->items)) != NULL)
   {
      list_remove(dev->items, devitem);
      os_free(devitem);
   }

   if (dev->name != NULL)
      os_free(dev->name);

   os_free(dev);

   return 0;
}

/** Update dmx buffer and send it to device */
static int refresh_dmx_buffer(dmx_device_item_t *devitem)
{
   int ix;
   
   // Set channels value to DMX output buffer
   for (ix = 0; ix < devitem->channels_count; ix++)
   {
      ASSERT(devitem->channels[ix] < CFG_DMX_NUM_CHANNELS);
      devitem->dev->dmxbuf[devitem->channels[ix]] = devitem->channels_values[ix];
   }

   VERIFY(osSemaphoreRelease(poll_sem) == osOK);

   return 0;
}

/** Start DMX fader */
static int dmx_fader_start(dmx_device_item_t *devitem, dmx_cmd_t *cmd, int periodical)
{
   int fadetime;
   
   osMutexWait(devitem->fader.mutex, osWaitForever);

   // Create a new timer
   if (devitem->fader.timer == NULL)
   {
      const osTimerDef(FADER_TIMER, dmx_fader_timer_cb);

      // Create click length timer
      if ((devitem->fader.timer = osTimerCreate(osTimer(FADER_TIMER), osTimerPeriodic, devitem)) == NULL)
      {
         TRACE_ERROR("Start fader timer");
         throw_exception(fail);
      }
   }

   if (periodical)
   {
      // Set max range of values
      cmd->channel_values[0] = cmd->default_channel_values[0];
      fadetime = periodical_fadetime;
      devitem->fader.sectimer = 0;
   }
   else
   {
      fadetime = cmd->fadetime;
   }

   devitem->fader.periodical = periodical;
   devitem->fader.interval = fadetick;
   devitem->fader.count = devitem->fader.maxcount = fadetime / devitem->fader.interval;
   devitem->fader.endvalue = -1;

   if (cmd->cmdtype == UHAB_ITEM_STATE_CMD_ON)
   {
      if (cmd->channel_values[0] > devitem->channels_values[0])
      {
         // From 0 -> max
         devitem->fader.step = cmd->channel_values[0] / devitem->fader.count;
         if (devitem->fader.step == 0)
            devitem->fader.step++;
            
         devitem->fader.endvalue = cmd->channel_values[0];
      }
      else
      {
         // From max -> current
         devitem->fader.step = devitem->channels_values[0] / devitem->fader.count;         
         if (devitem->fader.step == 0)
            devitem->fader.step++;

         devitem->fader.step *= -1;
         devitem->fader.endvalue = cmd->channel_values[0];
      }
   }
   else
   {
      // From current to zero
      devitem->fader.step = devitem->channels_values[0] / devitem->fader.count;
      if (devitem->fader.step == 0)
         devitem->fader.step++;

      devitem->fader.step *= -1;
      devitem->fader.endvalue = 0;
   }

   VERIFY(osTimerStart(devitem->fader.timer, devitem->fader.interval) == osOK);
   devitem->fader.start_time = hal_time_ms();
   
   osMutexRelease(devitem->fader.mutex);

   //TRACE("Start fader interval: %d   count: %d  step: %d  periodical: %d  cv: %d", 
   //   devitem->fader.interval, devitem->fader.count, devitem->fader.step, periodical, devitem->channels_values[0]);

   return 0;

fail:
   osMutexRelease(devitem->fader.mutex);
   return -1;
}

/** Stop DMX fader */
static dmx_cmd_t *dmx_fader_stop(dmx_device_item_t *devitem)
{
   dmx_cmd_t *cmd = NULL;
  
   osMutexWait(devitem->fader.mutex, osWaitForever);
   
   if (devitem->fader.timer == NULL)
   {
      TRACE_ERROR("Fader timer is not running");
      throw_exception(fail);
   }

   // Stop timer
   VERIFY(osTimerDelete(devitem->fader.timer) == osOK);
   devitem->fader.timer = NULL;
   devitem->fader.periodical = 0;
   
   // Find ON command
   if ((cmd = find_dmx_command(devitem, UHAB_ITEM_STATE_CMD_ON)) == NULL)
   {
      TRACE_ERROR("Not defined CMD command");
      throw_exception(fail);
   }

   // Save current DMX value 
   cmd->channel_values[0] = devitem->channels_values[0];
   
   osMutexRelease(devitem->fader.mutex);

   //TRACE("DMX fader stop, cv: %d  fadelen: %d ms", cmd->channel_values[0], hal_time_ms() - devitem->fader.start_time);

   return cmd;

fail:   
   osMutexRelease(devitem->fader.mutex);   
   return NULL;   
}

/** Return state of dmx fader */
static int dmx_fader_is_running_down(dmx_device_item_t *devitem)
{
   int res;
   
   osMutexWait(devitem->fader.mutex, osWaitForever);                     
   res = (devitem->fader.timer != NULL && devitem->fader.step < 0);
   osMutexRelease(devitem->fader.mutex);
   
   return res;
}

/** DMX fader timer callback */
static void dmx_fader_timer_cb(void *arg)
{
   dmx_device_item_t *devitem = arg;
   int value;

   osMutexWait(devitem->fader.mutex, osWaitForever);

   // Increase fader DMX channel value
   value = devitem->channels_values[0] + devitem->fader.step;
  
   // Check ranges and if it is out of range then stop fader
   if (devitem->fader.step > 0)
   {
      if (value >= devitem->fader.endvalue || devitem->fader.count == 0)
      {
         value = devitem->fader.endvalue;
         devitem->fader.count = 0;
      }
   }
   else
   {
      if (value <= devitem->fader.endvalue || devitem->fader.count == 0)
      {
         value = devitem->fader.endvalue;
         devitem->fader.count = 0;
      }
   }

   // Update fader value to DMX output buffer
   devitem->channels_values[0] = value;
   refresh_dmx_buffer(devitem);

   // Update item value per second in periodical mode
   if (devitem->fader.periodical)
   {
      if (hal_time_ms() >= devitem->fader.sectimer)
      {
         uhab_item_state_t newstate;

         // Update current value in %
         uhab_item_state_set_number(&newstate, round(devitem->channels_values[0] / (CFG_DMX_MAX_CHANNEL_VALUE / 100.0)));
         uhab_bus_update(devitem->item, &newstate);
         
         devitem->fader.sectimer = hal_time_ms() + CFG_DMX_PERIODICAL_UPDATE_ITEM_VALUE_TIME;
      }
   }

   // Decrement fader counter 
   devitem->fader.count--;
   if (devitem->fader.count < 0)
   {
      if (devitem->fader.periodical)
      {
         // Restart fader
         devitem->fader.count = devitem->fader.maxcount;
         devitem->fader.step *= -1;
         devitem->fader.endvalue = (devitem->fader.step > 0) ? 255 : 0;
         //TRACE("Restart periodical fader");
      }
      else
      {
         // Stop fader timer
         if (devitem->fader.timer != NULL)
         {
            VERIFY(osTimerDelete(devitem->fader.timer) == osOK);
            devitem->fader.timer = NULL;
            hal_gpio_set(HAL_GPIO2, 0);
            //TRACE("Fade done, cv: %d count: %d  fadelen: %d (ms)", devitem->channels_values[0], devitem->fader.count, hal_time_ms() - devitem->fader.start_time);
         }
      }
   }

   osMutexRelease(devitem->fader.mutex);
}


/** DMX poll timer callback */
static void dmx_poll_timer_cb(void *arg)
{
   VERIFY(osSemaphoreRelease(poll_sem) == osOK);
}

/** DMX poll thread */
static void dmx_poll_thread(void *arg)
{
   dmx_device_t *dev = arg;

   while(1)
   {
      if (osSemaphoreWait(poll_sem, osWaitForever) == osOK)
      {
         // For all DMX devices
         for (dev = list_head(dmx_devices); dev != NULL; dev = list_item_next(dev))
         {
            // Write output buffer to DMX device
            if (hal_dmx_write(dev->dmx, dev->dmxbuf, dev->dmxbuf_length) != dev->dmxbuf_length)
            {
               TRACE_ERROR("Send DMX: %s frame, errno: %d", dev->name, errno);
            }
         }

         hal_gpio_toggle(GPIO_LED_DMX_POLL);  
      }
      else
      {
         TRACE_ERROR("Wait for poll_sem");
      }
   }
}


/** DMX binding interface definition */
uhab_protocol_binding_t dmx_binding =
{
   .name = CFG_DMX_BINDING_NAME,
   .label = "DMX protocol",
   .init = dmx_binding_init,
   .deinit = dmx_binding_deinit,
   .start = dmx_binding_start,
   .configure = dmx_binding_configure,
   .send_command = dmx_binding_send
};
