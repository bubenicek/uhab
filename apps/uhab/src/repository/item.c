/**
 * \file item.c         \brief uHAB repository item definition
 */
 
#include "uhab.h"

TRACE_TAG(repository_item);
#if !ENABLE_TRACE_REPOSITORY
#include "trace_undef.h"
#endif

/** Alloc item */
uhab_item_t *uhab_item_alloc(uhab_item_type_t type)
{
   uhab_item_t *item;

   if ((item = os_malloc(sizeof(uhab_item_t))) != NULL)
   {
      os_memset(item, 0, sizeof(uhab_item_t));
      LIST_STRUCT_INIT(item, child_items);
      LIST_STRUCT_INIT(item, automation.rules);
      item->type = type;      
   }
   
   return item;
}

/** Free item */
void uhab_item_free(uhab_item_t *item)
{
   // TODO: free all containers
   
   os_free(item);
}


/** Add item automation rule */
int uhab_item_add_rule(uhab_item_t *item, struct uhab_rule *rule)
{
   uhab_rule_t *r;
   
   // Find the same rule
   for (r = list_head(item->automation.rules); r != NULL; r = list_item_next(r))
   {
      if (r->event == rule->event)
      {
         TRACE_ERROR("Rule for event: %s is already defined", r->evtdef->name);
         return -1;
      }
   }

   list_add(item->automation.rules, rule);
   
   return 0;
}
