/**
 * \file binding.c         \brief uHAB protocol binding
 */

#include "uhab.h"

TRACE_TAG(binding);
#if !ENABLE_TRACE_BINDING
#include "trace_undef.h"
#endif


//
// Bindings interfaces
//
BINDING_NAME(system_binding);
BINDING_NAME(modbus_binding);
BINDING_NAME(dmx_binding);
BINDING_NAME(snmp_binding);
BINDING_NAME(vehabus_binding);
BINDING_NAME(http_binding);
BINDING_NAME(mining_binding);
BINDING_NAME(ebus_binding);


/** Supported protocol bindings */
static uhab_protocol_binding_t *bindings_table[] =
{
   &system_binding,
   &modbus_binding,
#if defined (CFG_SNMP_ENABLED) && (CFG_SNMP_ENABLED == 1)
   &snmp_binding,
#endif
   &dmx_binding,
   &vehabus_binding,
   &http_binding,
   &mining_binding,
   &ebus_binding,
};
#define BINDINGS_COUNT  (sizeof(bindings_table) / sizeof(uhab_protocol_binding_t *))

LIST(bindings);

/** Initialize all configured bindings */
int uhab_binding_init(void)
{
   int res = 0;
   int ix;

   list_init(bindings);

   for (ix = 0; ix < BINDINGS_COUNT; ix++)
   {
      if (bindings_table[ix]->init() == 0)
      {
         TRACE("Binding '%s' initialized", bindings_table[ix]->name);
         list_add(bindings, bindings_table[ix]);
      }
      else
      {
         TRACE_ERROR("Init binding '%s'", bindings_table[ix]->name);
         res--;
      }
   }

   if (res == 0)
   {
      TRACE("Bindings initialized, number of modules: %d", ix);
   }

   return res;
}

int uhab_binding_deinit(void)
{
   int ix;

   for (ix = 0; ix < BINDINGS_COUNT; ix++)
   {
      if (bindings_table[ix]->deinit() != 0)
      {
         TRACE_ERROR("Deinit binding '%s'", bindings_table[ix]->name);
      }
   }

   TRACE("Bindings deinitialized");

   return 0;
}

/** Start configured bindings */
int uhab_binding_start(void)
{
   int ix, res = 0;
   uhab_item_t *item;

   for (ix = 0; ix < BINDINGS_COUNT; ix++)
   {
      if (bindings_table[ix]->start() != 0)
      {
         TRACE_ERROR("Start binding '%s'", bindings_table[ix]->name);
         return -1;
      }
   }

   // Notify all items start event
   for (item = list_head(repository.items); item != NULL; item = list_item_next(item))
   {
      res += uhab_automation_process_event(&automation, UHAB_RULE_EVENT_START, item, &item->state);
   }

   if (res != 0)
   {
      TRACE_ERROR("Process UHAB_RULE_EVENT_START failed");
      return -1;
   }

   TRACE("Binding started");

   return 0;
}


/** Get protocol binding by name */
const uhab_protocol_binding_t *uhab_binding_get_by_name(const char *name)
{
   int ix;

   for (ix = 0; ix < BINDINGS_COUNT; ix++)
   {
      if (!strcmp(bindings_table[ix]->name, name))
         return bindings_table[ix];
   }

   return NULL;
}

/** Get all bindings */
list_t uhab_binding_get_all_bindings(void)
{
   return bindings;
}
