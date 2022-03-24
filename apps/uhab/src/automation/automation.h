/**
 * \file automation.h         \brief uHAB server automation
 */

#ifndef __UHAB_AUTOMATION_H
#define __UHAB_AUTOMATION_H

#include "rule.h"


typedef struct uhab_automation_script
{
   struct uhab_automation_script  *next;
   char *body;
   
} uhab_automation_script_t;


typedef struct
{
   uint8_t initialized;
   
   /** Global javascript definition */
   LIST_STRUCT(scripts);
   
   /** Process mutex */
   osMutexId mutex;
   
} uhab_automation_t;


/** Initialize automation module */
int uhab_automation_init(uhab_automation_t *au);

/** Deinitialize automation module */
int uhab_automation_deinit(uhab_automation_t *au);

/** Process event rule handler */
int uhab_automation_process_event(uhab_automation_t *au, uhab_rule_event_t event, uhab_item_t *item, uhab_item_state_t *newstate);


#endif // __UHAB_AUTOMATION_H