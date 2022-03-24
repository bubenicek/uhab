/**
 * \file jscript.c         \brief Automation javascript interpreter
 */

#include "uhab.h"
#include "jscript.h"

TRACE_GROUP(automation);
#if !ENABLE_TRACE_JSCRIPT
#include "trace_undef.h"
#endif

// Prototypes:
static enum v7_err js_systime(struct v7 *v7, v7_val_t *res);
static enum v7_err js_abort(struct v7 *v7, v7_val_t *res);
static enum v7_err js_trace(struct v7 *v7, v7_val_t *res);
static enum v7_err js_trace_error(struct v7 *v7, v7_val_t *res);

// Locals:
static struct v7 *v7 = NULL;
static osMutexId jscript_mutex;

int uhab_jscript_init(uhab_automation_t *au)
{
   v7_val_t result;

   // Generate javascript rules
   if (uhab_jscript_generate(au, CFG_UHAB_RULES_JSCRIPT_FILENAME) != 0)
   {
      TRACE_ERROR("Generate javascript rules");
      throw_exception(fail_generate);
   }
   
   if ((jscript_mutex = osMutexCreate(NULL)) == NULL)
   {
      TRACE_ERROR("Create jscript mutex");
      throw_exception(fail_mutex);
   }
   
   // Create js engine
   if ((v7 = v7_create()) == NULL)
   {
      TRACE_ERROR("Create javascript engine");
      throw_exception(fail_create_v7);
   }
   
   // Initialize javascript timer object
   if (jscript_timer_init(v7) != 0)
   {
      TRACE_ERROR("jscript timer init failed");
      throw_exception(fail_init_objects);
   }

   if (jscript_items_init(v7) != 0)
   {
      TRACE_ERROR("jscript items init failed");
      throw_exception(fail_init_objects);
   }
   
   // Register global methods
   VERIFY(v7_set_method(v7, v7_get_global(v7), "systime", &js_systime) == V7_OK);
   VERIFY(v7_set_method(v7, v7_get_global(v7), "abort", &js_abort) == V7_OK);
   VERIFY(v7_set_method(v7, v7_get_global(v7), "TRACE", &js_trace) == V7_OK);
   VERIFY(v7_set_method(v7, v7_get_global(v7), "TRACE_ERROR", &js_trace_error) == V7_OK);
   
   uhab_jscript_lock();
   
   // Exec rules script
   if (v7_exec_file(v7, CFG_UHAB_RULES_JSCRIPT_FILENAME, &result) != V7_OK)
   {
      TRACE_ERROR("Evaluation error");
      v7_print_error(stderr, v7, "Evaluation error", result);
      throw_exception(fail_exec);
   }

   uhab_jscript_unlock();
   
   TRACE("Javascript engine init");
  
   return 0;

fail_exec:
   uhab_jscript_unlock();
fail_init_objects:
   v7_destroy(v7);
fail_create_v7:
   osMutexDelete(jscript_mutex);
fail_mutex:
fail_generate:
   return -1;   
}

int uhab_jscript_deinit(void)
{
   uhab_jscript_lock();

   if (v7 != NULL)
   {
      v7_destroy(v7);
   }

   uhab_jscript_unlock();

   osMutexDelete(jscript_mutex);
   
   return 0;
}

int uhab_jscript_execute(uhab_item_t *item, const char *funcname, uhab_item_state_t *newstate)
{
   v7_val_t func, result;
   v7_val_t value;
   
   ASSERT(item != NULL);
   ASSERT(funcname != NULL);

   uhab_jscript_lock();
   
   // Execute JS event handler if exists
   if ((func = v7_get(v7, v7_get_global(v7), funcname, ~0)) != V7_UNDEFINED)
   {
      if (v7_apply(v7, func, V7_UNDEFINED, V7_UNDEFINED, &result) != V7_OK) 
      {
         v7_print_error(stderr, v7, "Error: ", result);
         throw_exception(fail);
      }     

      // Get item JS state
      value = v7_get(v7, item->automation.jsobject, "state", ~0);    
      jsvalue_to_itemstate(v7, &value, newstate);
   }
   
   uhab_jscript_unlock();
   
   return 0;
   
fail:
   uhab_jscript_unlock();
   return -1;
}


int uhab_jscript_update(uhab_item_t *item, uhab_item_state_t *newstate)
{
   v7_val_t value;
   
   ASSERT(item != NULL);

   uhab_jscript_lock();
   
   // Save previous JS state
   value = v7_get(v7, item->automation.jsobject, "state", ~0);
   v7_set(v7, item->automation.jsobject, "prevstate", ~0, value);

   // Set new JS state value
   if (itemstate_to_jsvalue(v7, newstate, &value) != 0)
   {
      TRACE_ERROR("conversion itemstate -> jscript value");
      throw_exception(fail);
   }
   v7_set(v7, item->automation.jsobject, "state", ~0, value);
  
   uhab_jscript_unlock();
   
   return 0;
   
fail:
   uhab_jscript_unlock();
   return -1;
}


/** Lock access to jscript */
void uhab_jscript_lock(void)
{
   osMutexWait(jscript_mutex, osWaitForever);
}

/** Unlock access to jscript */
void uhab_jscript_unlock(void)
{
   osMutexRelease(jscript_mutex);
}



static enum v7_err js_systime(struct v7 *v7, v7_val_t *res)
{
   *res = v7_mk_number(v7, hal_time_ms());
   return V7_OK;
}

static enum v7_err js_abort(struct v7 *v7, v7_val_t *res)
{
   TRACE("System aborted !!");
   exit(1);
   return V7_OK;
}

static enum v7_err js_trace(struct v7 *v7, v7_val_t *res)
{
   v7_val_t value;

   value = v7_arg(v7, 0);
   if (value != V7_UNDEFINED && value != V7_NULL)
   {
      size_t len;
      TRACE("%s", v7_get_string(v7, &value, &len));
   }
   
   return V7_OK;
}

static enum v7_err js_trace_error(struct v7 *v7, v7_val_t *res)
{
   v7_val_t value;

   value = v7_arg(v7, 0);
   if (value != V7_UNDEFINED && value != V7_NULL)
   {
      size_t len;
      TRACE_ERROR("%s", v7_get_string(v7, &value, &len));
   }
   
   return V7_OK;
}
