/**
 * \file snmp_binding.c        \brief SNMP bindings
 */
 
#include "uhab.h"
#include "snmp_binding.h"

#if defined (CFG_SNMP_ENABLED) && (CFG_SNMP_ENABLED == 1)

TRACE_TAG(binding_snmp);
#if !ENABLE_TRACE_SNMP
#include "trace_undef.h"
#endif


// Prototypes:
static snmp_device_t *alloc_snmp_device(const char *name);
static int free_snmp_device(snmp_device_t *dev);
static void snmp_poll_thread(void *arg);

// Locals:
static const osThreadDef(SNMP_THREAD, snmp_poll_thread, CFG_SNMP_THREAD_PRIORITY, 0, CFG_SNMP_THREAD_STACK_SIZE);   
static osThreadId poll_thread;

LIST(snmp_devices);
static uint32_t poll_interval = CFG_SNMP_POLL_INTERVAL;


/** Initialize binding */
static int snmp_binding_init(void)
{ 
   list_init(snmp_devices);   
   TRACE("Init"); 
   
   return 0;
}

/** Deinitialize binding */
static int snmp_binding_deinit(void)
{
   // TODO:
   
   TRACE("Deinit");
   
   return 0;
}

/** Start binding */
static int snmp_binding_start(void)
{
   int devcount = 0;
   snmp_device_t *dev = NULL;
   
   // Initialialize all devices
   for (dev = list_head(snmp_devices); dev != NULL; dev = list_item_next(dev))
   {
      if (snmpc_open(&dev->conn, dev->hostname, dev->comunity) != 0)
      {
         TRACE_ERROR("Init SNMP connection hostname: %s", dev->hostname);
         return -1;
      }
      
      if (poll_interval > dev->poll_interval)
         poll_interval = dev->poll_interval;
         
      devcount++;
   }   

   if (devcount > 0)
   {
      // Start poll thread
      if ((poll_thread = osThreadCreate(osThread(SNMP_THREAD), NULL)) == 0)
      {
         TRACE_ERROR("Start poll thread");
         return -1;
      }   

      TRACE("Start");
   }
   
   return 0;
}

/** Configure binding */
static int snmp_binding_configure(struct uhab_item *item, const char *binding_config)
{
   char *pb, *pe;
   snmp_device_t *dev;
   snmp_device_item_t *devitem;
   char devname[255];
   char oid[255];
   
   // Parse binding config string modbus="slave1:0"
   if ((pb = strchr(binding_config, '=')) == NULL)
      throw_exception(fail_syntax);
   
   pb++;
   if ((pe = strchr(pb, ':')) == NULL)
      throw_exception(fail_syntax);
   
   if (pe - pb >= sizeof(devname))
      throw_exception(fail_syntax);

   // Device name      
   os_strncpy(devname, pb, pe - pb);
   devname[pe-pb] = '\0';

   // OID name
   os_strlcpy(oid, ++pe, sizeof(oid));
      
   // Create device or find exists
   if ((dev = alloc_snmp_device(devname)) == NULL)
   {
      TRACE_ERROR("Create SNMP device '%s'", devname);
      throw_exception(fail);
   }
   
   if ((devitem = os_malloc(sizeof(snmp_device_item_t))) == NULL)
   {
      TRACE_ERROR("Alloc device item");
      throw_exception(fail);
   }
   os_memset(devitem, 0, sizeof(snmp_device_item_t));
   devitem->item = item;
   if ((devitem->oid = os_strdup(oid)) == NULL)
   {
      TRACE_ERROR("Alloc oid");
      throw_exception(fail);
   }
   list_add(dev->items, devitem);
  
   TRACE("Configure item: '%s' [%s]   '%s'", item->name, binding_config, devname);
   
   return 0;

fail_syntax:
   TRACE_ERROR("Syntax error SNMP binding config '%s'", binding_config);
fail:
   return -1;
}

/** Send command (item state) to binded devices */
static int snmp_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   TRACE_ERROR("Send to item: %s not implemented", item->name);
   return -1;
}


/** Alloc new or use existing device */
static snmp_device_t *alloc_snmp_device(const char *name)
{
   char key[255];
   char value[255];
   snmp_device_t *dev = NULL;
   
   // Try to find exists device
   for (dev = list_head(snmp_devices); dev != NULL; dev = list_item_next(dev))
   {
      if (!strcmp(dev->name, name))
         break;
   }
   
   // Device not exists, alloc new
   if (dev == NULL)
   {
      if ((dev = os_malloc(sizeof(snmp_device_t))) == NULL)
      {
         TRACE_ERROR("Alloc device name");
         throw_exception(fail_alloc_dev);
      }
      os_memset(dev, 0, sizeof(snmp_device_t));
      LIST_STRUCT_INIT(dev, items);
      dev->name = os_strdup(name);
   
      // Get hostname
      snprintf(key, sizeof(key), "%s.hostname", name);
      if (uhab_config_service_get_value(CFG_SNMP_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         throw_exception(fail_key);
      }
      dev->hostname = os_strdup(value);

      // Comunity
      snprintf(key, sizeof(key), "%s.comunity", name);
      if (uhab_config_service_get_value(CFG_SNMP_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         throw_exception(fail_key);
      }
      dev->comunity = os_strdup(value);

      // Get poll time
      snprintf(key, sizeof(key), "%s.poll", name);
      if (uhab_config_service_get_value(CFG_SNMP_BINDING_NAME, key, value, sizeof(value)) != 0)
      {
         TRACE_ERROR("Not found key '%s'", key);   
         throw_exception(fail_key);
      }
      dev->poll_interval = atoi(value);

      list_add(snmp_devices, dev);
      TRACE("Alloc SNMP device name: %s   hostname: %s  polltime: %d", dev->name, dev->hostname, dev->poll_interval);
   }
  
   return dev;   

fail_key:
   free_snmp_device(dev);
fail_alloc_dev:
   return NULL;
}

static int free_snmp_device(snmp_device_t *dev)
{
   os_free(dev);
   return 0;
}

/** Working thread */
static void snmp_poll_thread(void *arg)
{
   snmp_device_t *dev;
   snmp_device_item_t *devitem;
   
   TRACE("SNMP thread is running ...  (poll_interval: %d ms)", poll_interval);
   
   while(1)
   {
      // Poll all devices
      for (dev = list_head(snmp_devices); dev != NULL; dev = list_item_next(dev))
      {
         // Test poll timeout
         if (hal_time_ms() < dev->poll_tmo)
            continue;      
            
         // Read all items OIDs
         for (devitem = list_head(dev->items); devitem != NULL; devitem = list_item_next(devitem))
         {
            switch(devitem->item->state.type)
            {
               case UHAB_ITEM_STATE_TYPE_NUMBER:
               {
                  int value;
                  uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_NUMBER);

                  if (snmpc_get_int_value(&dev->conn, devitem->oid, &value) == 0)
                  {
                     uhab_item_state_set_number(&newstate, value);

                     // Update item only when it was changed
                     if (uhab_item_state_compare(&devitem->state, &newstate) != 0)
                     {
                        if (uhab_bus_update(devitem->item, &newstate) != 0)
                        {
                           TRACE_ERROR("Update item failed");
                        }
                        
                        uhab_item_state_set(&devitem->state, &newstate);
                     }
                  }
                  else
                  {
                     TRACE_ERROR("Read device: %s OID: %s failed", dev->name, devitem->oid);
                  }
               }
               break;
               
               case UHAB_ITEM_STATE_TYPE_STRING:
               {
                  char value[255];
                  uhab_item_state_t newstate = UAHB_ITEM_STATE_INIT_STRING_REF(value);

                  if (snmpc_get_str_value(&dev->conn, devitem->oid, value, sizeof(value)) == 0)
                  {
                     // Update item only when it was changed
                     if (uhab_item_state_compare(&devitem->state, &newstate) != 0)
                     {                           
                        if (uhab_bus_update(devitem->item, &newstate) != 0)
                        {
                           TRACE_ERROR("Update item failed");
                        }
                        
                        // Save current state
                        uhab_item_state_set(&devitem->state, &newstate);
                     }
                  }
                  else
                  {
                     TRACE_ERROR("Read device: %s OID: %s failed", dev->name, devitem->oid);
                  }
               }
               break;
               
               default:
                  TRACE_ERROR("Not suported state type: %d", devitem->item->state.type);
            }
         }
         
         // Set next poll timeout
         dev->poll_tmo = hal_time_ms() + dev->poll_interval;
      }
      
      osDelay(poll_interval);
   }
}


/** Binding interface definition */
uhab_protocol_binding_t snmp_binding = 
{
   .name = CFG_SNMP_BINDING_NAME,
   .label = "SNMP protocol",
   .init = snmp_binding_init,
   .deinit = snmp_binding_deinit,
   .start = snmp_binding_start,
   .configure = snmp_binding_configure,
   .send_command = snmp_binding_send
};

#endif   // CFG_SNMP_ENABLED