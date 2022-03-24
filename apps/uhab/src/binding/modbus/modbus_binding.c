
#include "uhab.h"
#include "modbus_binding.h"
#include "modbus.h"

TRACE_TAG(binding_modbus);
#if !ENABLE_TRACE_MODBUS
#include "trace_undef.h"
#endif


// Prototypes:
static modbus_device_t *alloc_modbus_device(const char *name);
static void modbus_poll_thread(void *arg);

// Locals:
static const osThreadDef(MODBUS_POLL, modbus_poll_thread, CFG_MODBUS_THREAD_PRIORITY, 0, CFG_MODBUS_THREAD_STACK_SIZE);   
static osThreadId poll_thread;

static osPoolDef(CMD_POOL, CFG_MODBUS_BINDING_CMD_QUEUE_SIZE, modbus_cmd_t);
static osPoolId  cmd_pool;

const osMessageQDef(CMD_QUEUE, CFG_MODBUS_BINDING_CMD_QUEUE_SIZE, uint32_t);
osMessageQId cmd_queue;

static modbus_device_t devices[CFG_MODBUS_BINDING_MAX_DEVICES];
static int devices_count;

static int poll_interval = CFG_MODBUS_DEFAULT_POLL_INTERVAL;


//
// UHAB binding interface
//

/** Initialize binding */
static int modbus_binding_init(void)
{
   char txt[255];

   memset(&devices, 0, sizeof(devices));
   devices_count = 0;
   
   if ((cmd_queue = osMessageCreate(osMessageQ(CMD_QUEUE), osThreadGetId())) == NULL)
   {
      TRACE_ERROR("Create cmd queue");
      return -1;
   }
   
   if ((cmd_pool = osPoolCreate(osPool(CMD_POOL))) == NULL)
   {
      TRACE_ERROR("Create cmd pool");
      return -1;
   } 
   
   // Get configuration
   if (uhab_config_service_get_value(CFG_MODBUS_BINDING_NAME, "poll", txt, sizeof(txt)) == 0)
      poll_interval = atoi(txt);
   
   TRACE("Init"); 
   
   return 0;
}


/** Deinitialize binding */
static int modbus_binding_deinit(void)
{
   // TODO: free alocated resources
   
   TRACE("Deinit");
   return 0;
}

/** Start binding */
static int modbus_binding_start(void)
{
   int ix;
   
   // Initialize all devices
   for (ix = 0; ix < devices_count; ix++)
   {      
      switch(devices[ix].bus_type)
      {
         case MODBUS_DEVICE_BUS_SERIAL:
         {
            if (modbus_rtu_init(devices[ix].serial.uart, devices[ix].serial.baudrate, 0, devices[ix].serial.receive_timeout) != 0)
            {
               TRACE_ERROR("Open UART[%d] failed", devices[ix].serial.uart);
               return -1;
            }
         }
         break;
         
         default:
            TRACE_ERROR("Not supported modbus BUS type: %d", devices[ix].bus_type);
            return -1;
      }
      
      if (devices[ix].poll_interval < poll_interval)
         poll_interval = devices[ix].poll_interval;
   }
   
   if (devices_count > 0)
   {
      // Start poll thread
      if ((poll_thread = osThreadCreate(osThread(MODBUS_POLL), NULL)) == 0)
      {
         TRACE_ERROR("Start poll thread");
         return -1;
      }

      TRACE("Start");
   }

   return 0;
}

/** Configure binding */
static int modbus_binding_configure(struct uhab_item *item, const char *binding_config)
{
   char *pb, *pe;
   int item_index;
   modbus_device_t *dev;
   char txt[128];
   char name[128];
   
   // Parse binding config string modbus="slave1:0"
   if ((pb = strchr(binding_config, '=')) == NULL)
      throw_exception(fail_syntax);
   
   pb++;
   if ((pe = strchr(pb, ':')) == NULL)
      throw_exception(fail_syntax);
   
   if (pe - pb >= sizeof(name))
      throw_exception(fail_syntax);

   // Device name      
   strncpy(name, pb, pe - pb);
   name[pe-pb] = '\0';

   // Item index
   pb = ++pe;
   strcpy(txt, pb);
   item_index = atoi(txt);
      
   // Create modbus device or find exists
   if ((dev = alloc_modbus_device(name)) == NULL)
   {
      TRACE_ERROR("Create modbus device '%s'", name);
      throw_exception(fail);
   }
   
   // Assign item by index
   if (item_index >= CFG_MODBUS_BINDING_MAX_DEVICES)
   {
      TRACE_ERROR("Item bit index out of range 0-%d", CFG_MODBUS_BINDING_MAX_DEVICES);
      return -1;
   }
   
   dev->items[dev->items_count].dev = dev;
   dev->items[dev->items_count].item = item;
   dev->items[dev->items_count].index = item_index;
   uhab_item_state_set_command(&dev->items[dev->items_count].state, UHAB_ITEM_STATE_CMD_OFF);
   item->binding.protocol_item = &dev->items[dev->items_count];
   dev->items_count++;
   
   TRACE("Configure item: '%s' [%s]   '%s' %d", item->name, binding_config, name, item_index);
   return 0;

fail_syntax:
   TRACE_ERROR("Syntax error modbus binding config '%s'", binding_config);
fail:
   return -1;
}

/** Send command (item state) to binded devices */
static int modbus_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   modbus_cmd_t *cmd = NULL;
   modbus_device_item_t *devitem;
   
   ASSERT(item != NULL);
   ASSERT(state != NULL);
  
   devitem = item->binding.protocol_item;
   ASSERT(devitem != NULL);
  
   // Alloc command
   if ((cmd = osPoolAlloc(cmd_pool)) == NULL)
   {
      TRACE_ERROR("Alloc command");
      throw_exception(fail);
   }

   // Set command
   switch(devitem->dev->func_type)
   {
      case MODBUS_DEVICE_FUNC_COIL:
         cmd->type = MODBUS_CMD_SET_COIL;
         cmd->set_coil.dev = devitem->dev;
         cmd->set_coil.devitem = devitem;
         cmd->set_coil.coil = devitem->index;
         cmd->set_coil.state = state->value.cmd;
         break;
         
      case MODBUS_DEVICE_FUNC_HOLDING:
         cmd->type = MODBUS_CMD_WRITE_HOLDING;
         cmd->write_holding.dev = devitem->dev;
         cmd->write_holding.devitem = devitem;
         cmd->write_holding.regaddr = devitem->index;
         cmd->write_holding.regval = state->value.number;
         break;
         
      default:
         TRACE_ERROR("Item '%s' is not binded to output modbus device", item->name);
         throw_exception(fail);
   }      
   
   // Add command to queue   
   if (osMessagePut(cmd_queue, (uintptr_t)cmd, osWaitForever) != osOK)
   {
      TRACE_ERROR("Add cmd to queue");
      throw_exception(fail);
   }
   
   TRACE("Send command to '%s' state: %d", item->name, state->value.cmd);
   
   return 0;
   
fail:
   if (cmd != NULL)
      osPoolFree(cmd_pool, cmd);
   
   return -1;
}

/** Alloc or use exists modbus device */
static modbus_device_t *alloc_modbus_device(const char *name)
{
   int ix;
   char key[64];
   char value[64];
   int argc;
   char *argv[CFG_MODBUS_BINDING_CFG_MAX_ARGS];
   modbus_device_t *dev = NULL;
   
   // Try to find exists device
   for (ix = 0; ix < devices_count; ix++)
   {
      if (!strcmp(devices[ix].name, name))
      {
         dev = &devices[ix];
         break;
      }
   }
   
   // Device not exists, alloc new
   if (dev == NULL)
   {
      if (devices_count == CFG_MODBUS_BINDING_MAX_DEVICES)
      {
         TRACE_ERROR("Max number of modbus devices exceeded");
         return NULL;
      }
   
      dev = &devices[devices_count++];
      if ((dev->name = os_strdup(name)) == NULL)
      {
         TRACE_ERROR("Alloc device name");
         return NULL;
      }
   
      // Get device configuration
      snprintf(key, sizeof(key), "%s.connection", name);
      if (uhab_config_service_get_value(CFG_MODBUS_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         return NULL;
      }

      argc = split_line(value, ':', argv, CFG_MODBUS_BINDING_CFG_MAX_ARGS);
      if (argc != 5)
      {
         TRACE_ERROR("Bad number fo connection parameters");
         return NULL;
      }         
      
      if (!strcmp(argv[0], "serial"))
      {
         dev->bus_type = MODBUS_DEVICE_BUS_SERIAL;
      }         
      else
      {
         TRACE_ERROR("Not suported bus type: %s", argv[0]);
         return NULL;
      }
      
      dev->serial.uart = atoi(argv[1]);
      dev->serial.baudrate = atoi(argv[2]);
      dev->serial.inter_transaction_delay = atoi(argv[3]); 
      dev->serial.receive_timeout = atoi(argv[4]); 

      snprintf(key, sizeof(key), "%s.type", name);
      if (uhab_config_service_get_value(CFG_MODBUS_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         return NULL;
      }
      if (!strcmp(value, "coil"))
         dev->func_type = MODBUS_DEVICE_FUNC_COIL;
      else if (!strcmp(value, "discrete"))
         dev->func_type = MODBUS_DEVICE_FUNC_DISCRETE;
      else if (!strcmp(value, "holding"))
         dev->func_type = MODBUS_DEVICE_FUNC_HOLDING;
      else
      {
         TRACE_ERROR("Not supported type: %s of modbus device '%s'", value, dev->name);
         return NULL;
      }

      snprintf(key, sizeof(key), "%s.poll", name);
      if (uhab_config_service_get_value(CFG_MODBUS_BINDING_NAME, key, value, sizeof(value)) == 0)
      {
         dev->poll_interval = atoi(value);
      }
      
      snprintf(key, sizeof(key), "%s.id", name);
      if (uhab_config_service_get_value(CFG_MODBUS_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         return NULL;
      }
      dev->id = atoi(value);
      
      snprintf(key, sizeof(key), "%s.start", name);
      if (uhab_config_service_get_value(CFG_MODBUS_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         return NULL;
      }
      dev->start = atoi(value);

      snprintf(key, sizeof(key), "%s.length", name);
      if (uhab_config_service_get_value(CFG_MODBUS_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         return NULL;
      }
      dev->length = atoi(value);
      
      TRACE("Alloc modbus device: %s  bus: %d  func: %d  id: %d  start: %d  length: %d  poll: %d", 
         dev->name, dev->bus_type, dev->func_type, dev->id, dev->start, dev->length, dev->poll_interval);      
   }
   
   return dev;
}

/** Working thread */
static void modbus_poll_thread(void *arg)
{
   int ix;
   osEvent evt;
   modbus_device_t *dev;
      
   TRACE("Modbus poll thread is running ...  (poll_interval: %d ms)", poll_interval);
   
   while(1)
   {
      do 
      {
         //
         // Wait for commands in the queue or pool timeout
         //
         evt = osMessageGet(cmd_queue, poll_interval);
         if (evt.status == osEventMessage)
         {
            modbus_cmd_t *cmd = evt.value.p;
         
            switch(cmd->type)
            {
               case MODBUS_CMD_SET_COIL:
               {
                  dev = cmd->set_coil.dev;
                  uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_CMD);
               
                  if (dev->serial.inter_transaction_delay > 0)
                     hal_delay_ms(dev->serial.inter_transaction_delay);

                  if (modbus_rtu_write_coil(dev->serial.uart, dev->id, cmd->set_coil.coil, cmd->set_coil.state) != 0)
                  {
                     TRACE_ERROR("Write coil dev_addr: %d", dev->id);
                  }

                  // Update uhab item state
                  uhab_item_state_set_command(&newstate, cmd->set_coil.state);
                  VERIFY(uhab_bus_update(cmd->set_coil.devitem->item, &newstate) == 0);
                  uhab_item_state_set(&cmd->set_coil.devitem->state, &newstate);
                  
                  // Reset device poll timeout
                  dev->poll_tmo = 0;
               }
               break;
               
               case MODBUS_CMD_WRITE_HOLDING:
               {
                  dev = cmd->write_holding.dev;
                  uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_CMD);
               
                  if (dev->serial.inter_transaction_delay > 0)
                     hal_delay_ms(dev->serial.inter_transaction_delay);

                  if (modbus_rtu_write_sigle_register(dev->serial.uart, dev->id, cmd->write_holding.regaddr, cmd->write_holding.regval) != 0)
                  {
                     TRACE_ERROR("Write coil dev_addr: %d", dev->id);
                  }

                  // Update uhab item state
                  uhab_item_state_set_number(&newstate, cmd->write_holding.regval);
                  VERIFY(uhab_bus_update(cmd->write_holding.devitem->item, &newstate) == 0);
                  uhab_item_state_set(&cmd->write_holding.devitem->state, &newstate);

                  // Reset device poll timeout
                  dev->poll_tmo = 0;
               }
               break;
            
               default:
                  TRACE_ERROR("Not supported cmd type: %d", cmd->type);
            }
            
            osPoolFree(cmd_pool, cmd);
         } 
         else if (evt.status != osEventTimeout)
         {
            TRACE_ERROR("Get command from the queue");         
         }

      } while(evt.status == osEventMessage);

         
      //
      // Poll states
      //
      for (ix = 0; ix < devices_count; ix++)
      {
         dev = &devices[ix];
         
         // Test poll timeout
         if (hal_time_ms() < dev->poll_tmo)
            continue;
         
         if (dev->serial.inter_transaction_delay > 0)
            hal_delay_ms(dev->serial.inter_transaction_delay);
     
         switch(dev->func_type)
         {
            case MODBUS_DEVICE_FUNC_COIL:
            {
               int ib;
               uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_CMD);
               uint16_t state;
               
               // Get coils state
               if (modbus_rtu_read_coils_state(dev->serial.uart, dev->id, dev->start, dev->length, &state) == 0)
               {
                  for (ib = 0; ib < dev->items_count; ib++)
                  {
                     uhab_item_state_set_command(&newstate, ((state & (1 << ib)) != 0));
                     
                     // Update item only when it was changed
                     if (uhab_item_state_compare(&dev->items[ib].state, &newstate) != 0)
                     {
                        VERIFY(uhab_bus_update(dev->items[ib].item, &newstate) == 0);

                        // Save current state
                        uhab_item_state_set(&dev->items[ib].state, &newstate);
                     }
                  }
               }
               else
               {
                  TRACE_ERROR("Read coil state, dev_id: %d", dev->id);
               }
            }
            break;
            
            case MODBUS_DEVICE_FUNC_DISCRETE:
            {
               int ib;
               uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_CMD);
               uint16_t state;
               
               // Get discrete inpouts state
               if (modbus_rtu_read_inputs(dev->serial.uart, dev->id, dev->start, dev->length, &state) == 0)
               {
                  for (ib = 0; ib < dev->items_count; ib++)
                  {
                     uhab_item_state_set_command(&newstate, ((state & (1 << ib)) != 0));

                     // Update item only when it was changed
                     if (uhab_item_state_compare(&dev->items[ib].state, &newstate) != 0)
                     {
                        VERIFY(uhab_bus_update(dev->items[ib].item, &newstate) == 0);

                        // Save current state
                        uhab_item_state_set(&dev->items[ib].state, &newstate);
                     }
                  }
               }
               else
               {
                  TRACE_ERROR("Read discrete inputs state, dev_id: %d", dev->id);
               }
            }
            break;
            
            case MODBUS_DEVICE_FUNC_HOLDING:
            {
               int ix;
               uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_NUMBER);
               uint16_t regs[CFG_MODBUS_MAX_HOLDING_REGS_COUNT];
               
               // Get discrete inputs state
               if (modbus_rtu_read_holding_register(dev->serial.uart, dev->id, dev->start, dev->length, regs) == 0)
               {
                  for (ix = 0; ix < dev->items_count; ix++)
                  {
                     ASSERT(dev->items[ix].index < CFG_MODBUS_MAX_HOLDING_REGS_COUNT);
                     
                     uhab_item_state_set_number(&newstate, (double)regs[dev->items[ix].index]);

                     // Update item only when it was changed
                     if (uhab_item_state_compare(&dev->items[ix].state, &newstate) != 0)
                     {
                        VERIFY(uhab_bus_update(dev->items[ix].item, &newstate) == 0);

                        // Save current state
                        uhab_item_state_set(&dev->items[ix].state, &newstate);
                     }
                  }
               }
               else
               {
                  TRACE_ERROR("Read holding reg, dev_id: %d", dev->id);
               }
            }
            break;
            
            default:
               TRACE_ERROR("Not supported function type: %d", dev->func_type);
         }        
         
         // Set next poll timeout
         dev->poll_tmo = hal_time_ms() + dev->poll_interval;
      }
   }
}

/** Modbus binding interface definition */
uhab_protocol_binding_t modbus_binding = 
{
   .name = CFG_MODBUS_BINDING_NAME,
   .label = "MODBUS protocol",
   .init = modbus_binding_init,
   .deinit = modbus_binding_deinit,
   .start = modbus_binding_start,
   .configure = modbus_binding_configure,
   .send_command = modbus_binding_send
};

