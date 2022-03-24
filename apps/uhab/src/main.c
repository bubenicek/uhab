/**
 * \file main.c      \brief uHAB server entry point
 */

#include "uhab.h"

TRACE_TAG(uhab);
#if !ENABLE_TRACE_MAIN
#include "trace_undef.h"
#endif

// Prototypes:
static void uhab_system_init_thread(void *arg);

//
// Globals objects
//
uhab_system_status_t system_status;    // Systemn status
uhab_repository_t    repository;       // Items repository
uhab_automation_t    automation;       // Automation
uhab_uiprovider_t    uiprovider;       // UI provider

// Locals:
static const osThreadDef(INIT, uhab_system_init_thread, CFG_INIT_THREAD_PRIORITY, 0, CFG_INIT_THREAD_STACK_SIZE);


int main(void)
{
   // Initialize HW board
   VERIFY_FATAL(board_init() == 0);

   TRACE("uHAB server ver. %d.%d - Free heap size: %d/%d", CFG_FW_VERSION_MAJOR, CFG_FW_VERSION_MINOR, osMemGetFreeSize(), osMemGetTotalSize());

   // Initialize kernel
   osKernelInitialize();

   // Start init thread
   osThreadCreate(osThread(INIT), NULL);

   // Start scheduler
   osKernelStart();

   return 0;
}

/** Shutdown system */
void uhab_system_shutdown(void)
{
   TRACE("Shutdown system");
   board_deinit();
}

/** Restart system */
void uhab_system_restart(void)
{
   TRACE("Restart system");
   uhab_system_shutdown();
   hal_reset();
}

/** Reset to defaults */
void uhab_system_factory_reset(void)
{
   int fd;

   // Create factory reset command file
   if ((fd = open("factory_reset", O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0)
   {
      TRACE_ERROR("Can't create factory reset file");
      return;
   }
   close(fd);

   TRACE("Factory reset");

   uhab_system_restart();
}

/** System initialization thread */
static void uhab_system_init_thread(void *arg)
{
   int res = 0;

   // Clear status flags
   system_status.init_flags = 0;

   // 1) Initialize configuraton
   VERIFY_SYSTEM_INIT(uhab_config_init(), SYSTEM_INIT_CONFIG);

   // 2) Start BUS
   VERIFY_SYSTEM_INIT(uhab_bus_init(), SYSTEM_INIT_BUS_FLAG);

   // 3) Initialize protocols bindings
   VERIFY_SYSTEM_INIT(uhab_binding_init(), SYSTEM_INIT_BINDING_FLAG);

   // 4) Open items repository
   VERIFY_SYSTEM_INIT(uhab_repository_init(&repository), SYSTEM_INIT_REPOSITORY_FLAG);

   // 5) Initialize automation
   VERIFY_SYSTEM_INIT(uhab_automation_init(&automation), SYSTEM_INIT_AUTOMATION_FLAG);

   // 6) Initialize UI provider (must be always started !)
   SYSTEM_INIT(uhab_uiprovider_init(&uiprovider), SYSTEM_INIT_UIPROVIDER_FLAG);

   // 7) Start protocols bindings
   VERIFY_SYSTEM_INIT(uhab_binding_start(), SYSTEM_START_BINDING_FLAG);

   TRACE("uHAB server is running in %s mode", (system_status.init_flags == SYSTEM_READY_FLAGS) ? "normal" : "fail");

   // Terminate init thread
   VERIFY(osThreadTerminate(osThreadGetId()) == osOK);
}
