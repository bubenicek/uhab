/**
 * \file action.h       \brief Automation action definition
 */

#ifndef __UHAB_ACTION_H
#define __UHAB_ACTION_H

/** Action type */
typedef enum
{
   UHAB_ACTION_SEND_COMMAND,      // Send command
   UHAB_ACTION_DELAY,             // Delay in ms
   UHAB_ACTION_EXEC_JSCRIPT       // Execute javascript
   
} uhab_rule_action_type_t;


/** Action definition */
typedef struct
{
   char *name;
   uhab_rule_action_type_t type;
   
} uhab_rule_action_definition_t;


/** Rule action */
typedef struct uhab_rule_action
{
   struct uhab_rule_action *next;  
   
   /** Action definition */
   const uhab_rule_action_definition_t *actdef;
   
   /** Action type */
   uhab_rule_action_type_t type;
   
   /** Action condition */
   const char *condition;
   
   /** Action item */
   uhab_item_t *item;
   
   /** Action param string */
   const char *param;
   
} uhab_rule_action_t;


/** Alloc rule action */
uhab_rule_action_t *uhab_rule_action_alloc(void);

/** Free rule action */
void uhab_rule_action_free(uhab_rule_action_t *action);

/** Get rule action definition */
const uhab_rule_action_definition_t *uhab_rule_action_get_definition(const char *name);

#endif // __UHAB_ACTION_H