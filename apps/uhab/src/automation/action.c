/**
 * \file action.c       \brief Automation action definition
 */

#include "uhab.h"

/** Supported actions */
static const uhab_rule_action_definition_t uhab_rule_action_def[] = 
{
   {"send_command", UHAB_ACTION_SEND_COMMAND},
   {"delay", UHAB_ACTION_DELAY},
   {"script", UHAB_ACTION_EXEC_JSCRIPT},
   {NULL}
};

uhab_rule_action_t *uhab_rule_action_alloc(void)
{
   uhab_rule_action_t *action;
   
   if ((action = os_malloc(sizeof(uhab_rule_action_t))) != NULL)
   {
      os_memset(action, 0, sizeof(uhab_rule_action_t));
   }
   
   return action;
}

void uhab_rule_action_free(uhab_rule_action_t *action)
{
   os_free(action);
}


const uhab_rule_action_definition_t *uhab_rule_action_get_definition(const char *name)
{
   const uhab_rule_action_definition_t *def;
   
   for (def = &uhab_rule_action_def[0]; def->name != NULL; def++)
   {
      if (!strcasecmp(def->name, name))
         return def;
   }
   
   return NULL;
}