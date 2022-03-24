/**
 * \file item_state.c         \brief uHAB item state
 */
 
#include "uhab.h"

TRACE_TAG(repository_state);
#if !ENABLE_TRACE_REPOSITORY
#include "trace_undef.h"
#endif


/** String to item state command type */
int uhab_item_state_str2command(const char *str, uhab_item_state_cmd_t *cmd)
{
   int res = 0;
   
   if (!strcasecmp(str, "OFF"))
      *cmd = UHAB_ITEM_STATE_CMD_OFF;
   else if (!strcasecmp(str, "ON"))
      *cmd = UHAB_ITEM_STATE_CMD_ON;
   else if (!strcasecmp(str, "TOGGLE"))
      *cmd = UHAB_ITEM_STATE_CMD_TOGGLE;
   else if (!strcasecmp(str, "UP"))
      *cmd = UHAB_ITEM_STATE_CMD_UP;
   else if (!strcasecmp(str, "DOWN"))
      *cmd = UHAB_ITEM_STATE_CMD_DOWN;
   else if (!strcasecmp(str, "START"))
      *cmd = UHAB_ITEM_STATE_CMD_START;
   else if (!strcasecmp(str, "STOP"))
      *cmd = UHAB_ITEM_STATE_CMD_STOP;
   else
      res = -1;
   
   return res;
}

/** Get item state value as string */
const char *uhab_item_state_get_value_fmt(const uhab_item_state_t *state, const char *fmt, char *buf, int bufsize)
{
   switch(state->type)
   {
      case UHAB_ITEM_STATE_TYPE_CMD:
      {
         switch(state->value.cmd)
         {
            case UHAB_ITEM_STATE_CMD_OFF:
               snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "OFF");
               break;

            case UHAB_ITEM_STATE_CMD_ON:
               snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "ON");
               break;

            case UHAB_ITEM_STATE_CMD_TOGGLE:
               snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "TOGGLE");               
               break;

            case UHAB_ITEM_STATE_CMD_UP:
               snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "UP");
               break;

            case UHAB_ITEM_STATE_CMD_DOWN:
               snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "DOWN");
               break;

            case UHAB_ITEM_STATE_CMD_START:
               snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "START");
               break;

            case UHAB_ITEM_STATE_CMD_STOP:
               snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "STOP");
               break;
              
            default:
                snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "UNDEF");
               break;
         }
      }
      break;
         
      case UHAB_ITEM_STATE_TYPE_NUMBER:
      {
         double num;

         if (uhab_item_state_get_number(state, &num) != 0)
            return NULL;
         
         snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%g", num);   
      }
      break;
         
      case UHAB_ITEM_STATE_TYPE_STRING:
      {
         osMutexWait(repository.items_mutex, osWaitForever);
         snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", state->value.str != NULL ? state->value.str : "null");
         osMutexRelease(repository.items_mutex);
      }
      break;
         
      default:
         snprintf(buf, bufsize, (fmt != NULL) ? fmt : "%s", "UNDEF");
   }
  
   return buf;
}

const char *uhab_item_state_get_value(const uhab_item_state_t *state, char *buf, int bufsize)
{
   return uhab_item_state_get_value_fmt(state, NULL, buf, bufsize);
}

/** Set value from string by state type */
int uhab_item_state_set_value(uhab_item_state_t *state, const char *buf)
{
   int res;
   
   switch(state->type)
   {
      case UHAB_ITEM_STATE_TYPE_CMD:
         res = uhab_item_state_set_command(state, atoi(buf));
         break;

        
      case UHAB_ITEM_STATE_TYPE_NUMBER:
         res = uhab_item_state_set_number(state, atof(buf)); 
         break;
         
      case UHAB_ITEM_STATE_TYPE_STRING:
         res = uhab_item_state_set_string(state, buf);
         break;
         
      default:
         res = -1;
   }
   
   return res;
}


/** Compare item state */
int uhab_item_state_compare(const uhab_item_state_t *curstate, const uhab_item_state_t *newstate)
{
   int res = -1;
   
   ASSERT(curstate != NULL);
   ASSERT(newstate != NULL);
   
   osMutexWait(repository.items_mutex, osWaitForever);
   
   if (curstate->type == newstate->type)
   {
      switch(curstate->type)
      {
         case UHAB_ITEM_STATE_TYPE_CMD:
            res = (curstate->value.cmd == newstate->value.cmd) ? 0 : -1;
            break;
            
         case UHAB_ITEM_STATE_TYPE_NUMBER:
            res = (curstate->value.number == newstate->value.number) ? 0 : -1;
            break;
            
         case UHAB_ITEM_STATE_TYPE_STRING:
            if (curstate->value.str != NULL && newstate->value.str != NULL)
               res = strcmp(curstate->value.str, newstate->value.str);
            break;
            
         default:
            break;
      }
   }

   osMutexRelease(repository.items_mutex);
   
   return res;
}

/** Set item state */
int uhab_item_state_set(uhab_item_state_t *dest, const uhab_item_state_t *src)
{
   ASSERT(dest != NULL);
   ASSERT(src != NULL);
   
   // Release current state
   if (uhab_item_state_release(dest) != 0)
      return -1;

   osMutexWait(repository.items_mutex, osWaitForever);

   switch(src->type)
   {
      case UHAB_ITEM_STATE_TYPE_STRING:
         dest->type = UHAB_ITEM_STATE_TYPE_STRING;
         if (src->value.str != NULL)
         {
            if ((dest->value.str = os_strdup(src->value.str)) == NULL)
               throw_exception(fail);
         }
         break;
         
      default:
         *dest = *src;
         break;
   }

   osMutexRelease(repository.items_mutex);

   return 0;

fail:
   osMutexRelease(repository.items_mutex);
   return -1;
}

int uhab_item_state_set_command(uhab_item_state_t *state, uhab_item_state_cmd_t cmd)
{ 
   uhab_item_state_release(state);

   osMutexWait(repository.items_mutex, osWaitForever);

   state->type = UHAB_ITEM_STATE_TYPE_CMD;
   state->value.cmd = cmd;

   osMutexRelease(repository.items_mutex);

   return 0;
}

int uhab_item_state_get_command(const uhab_item_state_t *state, uhab_item_state_cmd_t *cmd)
{
   if (state->type != UHAB_ITEM_STATE_TYPE_CMD)
      return -1;

   osMutexWait(repository.items_mutex, osWaitForever);
   *cmd = state->value.cmd;
   osMutexRelease(repository.items_mutex);
   
   return 0;
}

int uhab_item_state_set_number(uhab_item_state_t *state, double number)
{ 
   uhab_item_state_release(state);

   osMutexWait(repository.items_mutex, osWaitForever);

   state->type = UHAB_ITEM_STATE_TYPE_NUMBER;
   state->value.number = number;

   osMutexRelease(repository.items_mutex);

   return 0;
}

int uhab_item_state_get_number(const uhab_item_state_t *state, double *number)
{
   ASSERT(number != NULL);
   
   if (state->type != UHAB_ITEM_STATE_TYPE_NUMBER)
      return -1;

   osMutexWait(repository.items_mutex, osWaitForever);
   *number = state->value.number;
   osMutexRelease(repository.items_mutex);

   return 0;
}

int uhab_item_state_set_string(uhab_item_state_t *state, const char *str)
{ 
   ASSERT(str != NULL);

   uhab_item_state_release(state);

   osMutexWait(repository.items_mutex, osWaitForever);

   state->type = UHAB_ITEM_STATE_TYPE_STRING;
   if ((state->value.str = os_strdup(str)) == NULL)
      throw_exception(fail);

   osMutexRelease(repository.items_mutex);

   return 0;
   
fail:   
   osMutexRelease(repository.items_mutex);
   return -1;
}

int uhab_item_state_get_string(const uhab_item_state_t *state, char *buf, int bufsize)
{
   if (state->type != UHAB_ITEM_STATE_TYPE_STRING)
      return -1;

   osMutexWait(repository.items_mutex, osWaitForever);
   snprintf(buf, bufsize, "%s", state->value.str != NULL ? state->value.str : "null");
   osMutexRelease(repository.items_mutex);
   
   return 0;
}

/** Shalow item state copy */
int uhab_item_state_copy(uhab_item_state_t *dest, const uhab_item_state_t *src)
{
   osMutexWait(repository.items_mutex, osWaitForever);
   *dest = *src;
   osMutexRelease(repository.items_mutex);

   return 0;
}

/** Release state allocated resource */
int uhab_item_state_release(uhab_item_state_t *state)
{
   ASSERT(state != NULL);

   osMutexWait(repository.items_mutex, osWaitForever);

   switch(state->type)
   {
      case UHAB_ITEM_STATE_TYPE_STRING:
         if (state->value.str != NULL)
            os_free(state->value.str);
         break;
         
      default:
         break;
   }

   os_memset(state, 0, sizeof(uhab_item_state_t));

   osMutexRelease(repository.items_mutex);

   return 0;
}
