/**
 * \file ebus_binding.c        \brief EBUS protocol binding
 */

#include "uhab.h"
#include "ebus_binding.h"

TRACE_TAG(binding_ebus);
#if !ENABLE_TRACE_EBUS
#include "trace_undef.h"
#endif

// Prototypes:
static void ebus_thread(void *arg);


// Locals:
static const osThreadDef(EBUS, ebus_thread, CFG_EBUS_THREAD_PRIORITY, 0, CFG_EBUS_THREAD_STACK_SIZE);
static osThreadId thread;

static osPoolDef(EBUS, CFG_EBUS_QUEUE_SIZE, uint32_t);
static osPoolId pool;

const osMessageQDef(EBUS, CFG_EBUS_QUEUE_SIZE, uint32_t);
static osMessageQId queue;


/** Initialize binding */
static int ebus_binding_init(void)
{
   if ((queue = osMessageCreate(osMessageQ(EBUS), osThreadGetId())) == NULL)
   {
      TRACE_ERROR("Create queue");
      throw_exception(fail_queue);
   }

   if ((pool = osPoolCreate(osPool(EBUS))) == NULL)
   {
      TRACE_ERROR("Create pool");
      throw_exception(fail_pool);
   }

   TRACE("Init");

   return 0;

fail_pool:
fail_queue:
   return -1;
}

/** Deinitialize binding */
static int ebus_binding_deinit(void)
{
   TRACE("Deinit");
   return 0;
}

/** Start binding */
static int ebus_binding_start(void)
{
   if ((thread = osThreadCreate(osThread(EBUS), NULL)) == 0)
   {
      TRACE_ERROR("Start thread");
      throw_exception(fail);
   }

   TRACE("Start");

   return 0;

fail:
   return -1;
}

/** Configure binding */
static int ebus_binding_configure(struct uhab_item *item, const char *binding_config)
{
    TRACE("Configure item: %s - %s", item->name, binding_config);
    return 0;
}

/** Send command (item state) to binded devices */
static int ebus_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   return 0;
}


static void ebus_thread(void *arg)
{
   TRACE("Working thread is running ...");

   while(1)
   {
        hal_delay_ms(1000);
   }
}


/** Interface definition */
uhab_protocol_binding_t ebus_binding =
{
   .name = CFG_EBUS_BINDING_NAME,
   .label = "EBUS",
   .init = ebus_binding_init,
   .deinit = ebus_binding_deinit,
   .start = ebus_binding_start,
   .configure = ebus_binding_configure,
   .send_command = ebus_binding_send
};
