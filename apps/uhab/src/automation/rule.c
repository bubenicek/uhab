/**
 * \file rule.c         \brief Automation rules definition
 */
 
#include "uhab.h"

/** Rules event definition */
static const uhab_rule_event_definition_t uhab_rule_events_def[] = 
{
   {"on", UHAB_RULE_EVENT_ON},
   {"off", UHAB_RULE_EVENT_OFF},
   {"click", UHAB_RULE_EVENT_CLICK},
   {"doubleclick", UHAB_RULE_EVENT_DOUBLECLICK},
   {"tripleclick", UHAB_RULE_EVENT_TRIPLECLICK},
   {"quadclick", UHAB_RULE_EVENT_QUADCLICK},
   {"longclick", UHAB_RULE_EVENT_LONGCLICK},
   {"longpress", UHAB_RULE_EVENT_LONGPRESS},
   {"changed", UHAB_RULE_EVENT_CHANGED},
   {"start", UHAB_RULE_EVENT_START},
   {"stop", UHAB_RULE_EVENT_STOP},
   {NULL}
};


uhab_rule_t *uhab_rule_alloc(void)
{
   uhab_rule_t *rule;
   
   if ((rule = os_malloc(sizeof(uhab_rule_t))) != NULL)
   {
      memset(rule, 0, sizeof(uhab_rule_t));
      LIST_STRUCT_INIT(rule, actions);
   }
   
   return rule;
}

void uhab_rule_free(uhab_rule_t *rule)
{
   os_free(rule);
}

const uhab_rule_event_definition_t *uhab_rule_get_definition(const char *name)
{
   const uhab_rule_event_definition_t *def;
   
   for (def = &uhab_rule_events_def[0]; def->name != NULL; def++)
   {
      if (!strcasecmp(def->name, name))
         return def;
   }
   
   return NULL;
}