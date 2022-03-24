/**
 * jscript_items.c         \brief Javascript items command implementation
 */
 
#include "uhab.h"
#include "jscript.h"

TRACE_GROUP(automation);
#if !ENABLE_TRACE_JSCRIPT
#include "trace_undef.h"
#endif

// Prototypes:
static enum v7_err js_item_send_command(struct v7 *v7, v7_val_t *res) ;
static enum v7_err js_item_update(struct v7 *v7, v7_val_t *res);


/** Initialize items module */
int jscript_items_init(struct v7 *v7)
{
   uhab_item_t *item;
   v7_val_t value = V7_NULL;
   
   // Define js item objects
   for (item = list_head(repository.items); item != NULL; item = list_item_next(item))
   {
      // Create static item object
      item->automation.jsobject = v7_mk_object(v7);     

      // Define object name
      v7_set(v7, v7_get_global(v7), item->name, ~0, item->automation.jsobject);         

      // Define property state
      if (itemstate_to_jsvalue(v7, &item->state, &value) != 0)
      {
         TRACE_ERROR("Set item: %s property state failed", item->name);
         return -1;
      }
      v7_def(v7, item->automation.jsobject, "state", ~0, (V7_DESC_GETTER(0)), value);
      v7_def(v7, item->automation.jsobject, "prevstate", ~0, (V7_DESC_GETTER(0)), value);

      // Define methods
      v7_set_method(v7, item->automation.jsobject, "send_command", js_item_send_command);
      v7_set_method(v7, item->automation.jsobject, "update", js_item_update);

      // Set user data
      v7_set_user_data(v7, item->automation.jsobject, item);
   }
   
   return 0;
}

/** Send command - item method */
static enum v7_err js_item_send_command(struct v7 *v7, v7_val_t *res)
{
   v7_val_t val;
   v7_val_t this_obj;
   uhab_item_t *item;
   
   this_obj = v7_get_this(v7); 

   // Get item pointer
   item = v7_get_user_data(v7, this_obj);
   ASSERT(item != NULL);

   // Get state value
   val = v7_arg(v7, 0);
  
   // Set item new state value
   uhab_item_state_t state = UHAB_ITEM_STATE_INIT(item->state.type);   
   
   // Convert jsvascript to item state
   if (jsvalue_to_itemstate(v7, &val, &state) != 0)
   {
      TRACE_ERROR("conversion jscript value -> item state failed");
      throw_exception(fail);
   }
     
   // Execute send
   if (uhab_bus_send(item, &state) != 0)
   {
      TRACE_ERROR("Send command to item: %s", item->name);
      throw_exception(fail);
   }
 
   uhab_item_state_release(&state);
   *res = v7_mk_number(v7, 0);
 
   return V7_OK;
   
fail:
   uhab_item_state_release(&state);
   *res = v7_mk_number(v7, -1);
   return V7_OK;  
}

/** Update - item method */
static enum v7_err js_item_update(struct v7 *v7, v7_val_t *res) 
{
   v7_val_t value;
   v7_val_t this_obj;
   uhab_item_t *item;
   uhab_item_state_t state = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_NONE);   
   
   this_obj = v7_get_this(v7); 

   // Get item pointer
   item = v7_get_user_data(v7, this_obj);
   ASSERT(item != NULL);

   // Get state value
   value = v7_arg(v7, 0);
      
   if (item->automation.event_pending)
   {     
      // Update js object item state only
      v7_set(v7, item->automation.jsobject, "state", ~0, value);      
   }
   else
   {
      // Convert jsvascript to item state
      if (jsvalue_to_itemstate(v7, &value, &state) != 0)
      {
         TRACE_ERROR("conversion jscript value -> item state failed");
         throw_exception(fail);
      }

      // Execute update
      if (uhab_bus_update(item, &state) != 0)
      {
         TRACE_ERROR("Send command to item: %s", item->name);
         throw_exception(fail);
      }            

      uhab_item_state_release(&state);
   }

   *res = v7_mk_number(v7, 0);
 
   return V7_OK;
   
fail:
   uhab_item_state_release(&state);
   *res = v7_mk_number(v7, -1);
   return V7_OK; 
}
