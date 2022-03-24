/**
 * \file rule.h         \brief Automation rules definition
 */

#ifndef __UHAB_RULE_H
#define __UHAB_RULE_H

#include "action.h"

/** Rule events */
typedef enum
{
   UHAB_RULE_EVENT_ON            = 0x01,
   UHAB_RULE_EVENT_OFF           = 0x02,
   UHAB_RULE_EVENT_CLICK         = 0x04,
   UHAB_RULE_EVENT_DOUBLECLICK   = 0x08,
   UHAB_RULE_EVENT_TRIPLECLICK   = 0x10,
   UHAB_RULE_EVENT_QUADCLICK     = 0x20,
   UHAB_RULE_EVENT_LONGCLICK     = 0x40,
   UHAB_RULE_EVENT_LONGPRESS     = 0x80,
   UHAB_RULE_EVENT_START         = 0x100,
   UHAB_RULE_EVENT_STOP          = 0x200,
   UHAB_RULE_EVENT_CHANGED       = (UHAB_RULE_EVENT_ON|UHAB_RULE_EVENT_OFF|UHAB_RULE_EVENT_CLICK|UHAB_RULE_EVENT_DOUBLECLICK|
                                    UHAB_RULE_EVENT_TRIPLECLICK|UHAB_RULE_EVENT_QUADCLICK|UHAB_RULE_EVENT_LONGCLICK|UHAB_RULE_EVENT_LONGPRESS)
   
} uhab_rule_event_t;


/** Rule event definition */
typedef struct
{
   char *name;
   uhab_rule_event_t type;
   
} uhab_rule_event_definition_t;


/** Automation rule */
typedef struct uhab_rule
{
   struct uhab_rule *next;   
   
   /** Rule event definition */
   const uhab_rule_event_definition_t *evtdef;
   
   /** Rule name */
   const char *name;

   /** Event type */
   uhab_rule_event_t event;
   
   /** Actions */
   LIST_STRUCT(actions);   

   /** Javascript automation function */
   const char *jscript_function;
   
} uhab_rule_t;


/** Alloc new rule */
uhab_rule_t *uhab_rule_alloc(void);

/** Free allocated rule */
void uhab_rule_free(uhab_rule_t *rule);

/** Get rule event definition */
const uhab_rule_event_definition_t *uhab_rule_get_definition(const char *name);


#endif // __UHAB_RULE_H