/**
 * \file jscript_utils.c         \brief Automation javascrit utils
 */

#include "uhab.h"
#include "jscript.h"

TRACE_GROUP(automation);
#if !ENABLE_TRACE_JSCRIPT
#include "trace_undef.h"
#endif

int itemstate_to_jsvalue(struct v7 *v7, const uhab_item_state_t *state, v7_val_t *value)
{
   switch(state->type)
   {
      case UHAB_ITEM_STATE_TYPE_CMD:
      {
         uhab_item_state_cmd_t cmd;
         
         if (uhab_item_state_get_command(state, &cmd) != 0)
            return -1;
         
         *value = v7_mk_number(v7, cmd + CFG_MAX_DOUBLE_VALUE);   
      }
      break;
         
      case UHAB_ITEM_STATE_TYPE_NUMBER:
      {
         double num;

         if (uhab_item_state_get_number(state, &num) != 0)
            return -1;
         
         *value = v7_mk_number(v7, num);
      }
      break;
         
      case UHAB_ITEM_STATE_TYPE_STRING:
      {
         char buf[255];

         if (uhab_item_state_get_string(state, buf, sizeof(buf)) != 0)
            return -1;

         *value = v7_mk_string(v7, buf, ~0, 1);
      }
      break;

      default:
         *value = V7_NULL;
   }

   return 0;
}

int jsvalue_to_itemstate(struct v7 *v7, v7_val_t *value, uhab_item_state_t *state)
{
   ASSERT(value != NULL);
   
   if (v7_is_number(*value))
   {
      double dval;
      
      dval = v7_get_double(v7, *value);
     
      if (dval < CFG_MAX_DOUBLE_VALUE)
      {
         // double is value range
         uhab_item_state_set_number(state, dval);
      }
      else
      {
         // double is command range
         uhab_item_state_set_command(state, dval - CFG_MAX_DOUBLE_VALUE);
      }
   }
   else if (v7_is_string(*value))
   {
      uhab_item_state_t newstate = UAHB_ITEM_STATE_INIT_STRING_REF(v7_get_string(v7, value, NULL));         

      if (uhab_item_state_compare(state, &newstate) != 0)
         uhab_item_state_set(state, &newstate);
   }
   else if (v7_is_null(*value))
   {
      uhab_item_state_release(state);
   }
   else
   {
      TRACE_ERROR("Not valid JS value of state type: %d", state->type);
      return -1;
   }
  
   return 0;
}
