
#ifndef __SNMP_BINDING_H
#define __SNMP_BINDING_H

#if defined (CFG_SNMP_ENABLED) && (CFG_SNMP_ENABLED == 1)

#include "snmpc.h"

#ifndef CFG_SNMP_POLL_INTERVAL
#define CFG_SNMP_POLL_INTERVAL         1000
#endif

/** SNMP device item */
typedef struct snmp_device_item
{
   struct snmp_device_item *next;
   
   /** UHAB item */
   const uhab_item_t *item;
   
   /** SNMP object ID */
   const char *oid;
   
   /** RAW state value */
   uhab_item_state_t state;
   
} snmp_device_item_t;


/** SNMP device */
typedef struct snmp_device
{
   struct snmp_device *next;
  
   const char *name;
   const char *hostname;
   const char *comunity;
   uint16_t poll_interval;
   uint32_t poll_tmo;

   snmpc_t conn;
   
   /** SNMP device items */
   LIST_STRUCT(items);
   
} snmp_device_t;

#endif   // CFG_SNMP_ENABLED


#endif // __SNMP_BINDING_H

