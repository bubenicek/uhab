/**
 * jscript_timer.c         \brief Javascript timer command implementation
 */
 
#include "uhab.h"
#include "jscript.h"

TRACE_GROUP(automation);
#if !ENABLE_TRACE_JSCRIPT
#include "trace_undef.h"
#endif

typedef struct
{
   struct v7 *v7;
   osTimerId id;
   v7_val_t func_cb;
   
} js_timer_t;


// Prototypes:
static enum v7_err js_timer_create(struct v7 *v7, v7_val_t *res);
static enum v7_err js_timer_destroy(struct v7 *v7, v7_val_t *res);
static enum v7_err js_timer_start(struct v7 *v7, v7_val_t *res);
static enum v7_err js_timer_stop(struct v7 *v7, v7_val_t *res);
static void js_timer_callback(void *arg);


int jscript_timer_init(struct v7 *v7)
{  
   VERIFY(v7_set_method(v7, v7_get_global(v7), "timer_create", &js_timer_create) == V7_OK);
   return 0;
}

static enum v7_err js_timer_create(struct v7 *v7, v7_val_t *result)
{
   int res;
   v7_val_t this_obj;
   v7_val_t value, func_cb;
   js_timer_t *timer = NULL;
   os_timer_type type;
   const osTimerDef(JS_TIMER, js_timer_callback);

   // Get input args
   value = v7_arg(v7, 0);
   if (value == V7_UNDEFINED || value == V7_NULL)
   {
      TRACE_ERROR("Timer.create type of timer param is expected");
      throw_exception(fail);
   }
   type = v7_get_int(v7, value);
   if (type != osTimerOnce && type != osTimerPeriodic)
   {
      TRACE_ERROR("Bad type of timer type: %d", type);
      throw_exception(fail);
   }

   func_cb = v7_arg(v7, 1);
   if (func_cb == V7_UNDEFINED || func_cb == V7_NULL)
   {
      TRACE_ERROR("Timer.create func callback param is expected");
      throw_exception(fail);
   }

   // Create and define new object
   this_obj = v7_mk_object(v7);
   res = v7_set_method(v7, this_obj, "destroy", &js_timer_destroy);
   res += v7_set_method(v7, this_obj, "start", &js_timer_start);
   res += v7_set_method(v7, this_obj, "stop", &js_timer_stop);
   
   if (res != 0)
   {
      TRACE_ERROR("Create timer object failed");
      throw_exception(fail);
   }
   
   if ((timer = os_malloc(sizeof(js_timer_t))) == NULL)
   {
      TRACE_ERROR("Alloc timer object");
      throw_exception(fail);
   }
   
   if ((timer->id = osTimerCreate(osTimer(JS_TIMER), type, timer)) == NULL)
   {
      TRACE_ERROR("Create timer failed");
      throw_exception(fail);
   }
   
   timer->v7 = v7;
   timer->func_cb = func_cb;

   v7_set_user_data(v7, this_obj, timer);
   *result = this_obj;

   TRACE("Create timer[%p]", timer);

   return V7_OK;
   
fail:
   if (timer != NULL)
      os_free(timer);
   *result = V7_NULL; 
   return V7_OK;
}

/** Delete timer */
static enum v7_err js_timer_destroy(struct v7 *v7, v7_val_t *res) 
{
   js_timer_t *timer;
   v7_val_t this_obj = v7_get_this(v7);

   // Get item pointer
   if ((timer = v7_get_user_data(v7, this_obj)) == NULL)
   {
      TRACE_ERROR("Not valid Timer object");
      throw_exception(fail);
   }
   
   osTimerDelete(timer->id);
   os_free(timer);
   v7_set_user_data(v7, this_obj, NULL);

   // Set output result
   *res = v7_mk_number(v7, 0);

   TRACE("Destroy timer[%X]", this_obj);
  
   return V7_OK;

fail:
   *res = v7_mk_number(v7, -1);
   return V7_OK;
}

/** Start timer */
static enum v7_err js_timer_start(struct v7 *v7, v7_val_t *res) 
{
   js_timer_t *timer;
   v7_val_t value;
   int interval;
   v7_val_t this_obj = v7_get_this(v7);
   
   value = v7_arg(v7, 0);
   if (value == V7_UNDEFINED || value == V7_NULL)
   {
      TRACE_ERROR("Timer.start interval param is expected");
      throw_exception(fail);
   }
   interval = v7_get_int(v7, value);
   
   // Get item pointer
   if ((timer = v7_get_user_data(v7, this_obj)) == NULL)
   {
      TRACE_ERROR("Not valid timer object");
      throw_exception(fail);
   }
   
   if (osTimerStart(timer->id, interval) != osOK)
   {
      TRACE_ERROR("Start timer failed");
      throw_exception(fail);
   }

   // Set output result
   *res = v7_mk_number(v7, 0);
  
   TRACE("Timer[%p] start, interval: %d", timer, interval);
   
   return V7_OK;

fail:
   // Set output result
   *res = v7_mk_number(v7, -1);
   return V7_OK;
}

static enum v7_err js_timer_stop(struct v7 *v7, v7_val_t *res) 
{
   js_timer_t *timer;
   v7_val_t this_obj = v7_get_this(v7);

   // Get item pointer
   if ((timer = v7_get_user_data(v7, this_obj)) == NULL)
   {
      TRACE_ERROR("Not valid timer object");
      throw_exception(fail);
   }
   
   if (osTimerStop(timer->id) != osOK)
   {
      TRACE_ERROR("Stop timer failed");
      throw_exception(fail);
   }

   // Set output result
   *res = v7_mk_number(v7, 0);
   
   TRACE("Timer[%p] stop", timer);
   
   return V7_OK;

fail:
   // Set output result
   *res = v7_mk_number(v7, -1);
   return V7_OK;
}


static void js_timer_callback(void *arg)
{
   js_timer_t *timer = arg;
   v7_val_t result;

   ASSERT(timer != NULL);

   uhab_jscript_lock();
   
   if (v7_apply(timer->v7, timer->func_cb, V7_UNDEFINED, V7_UNDEFINED, &result) != V7_OK) 
   {
      v7_print_error(stderr, timer->v7, "Error while calling timer callback\n", result);
   }

   uhab_jscript_unlock();
}
