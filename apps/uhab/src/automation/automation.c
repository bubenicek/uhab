/**
 * \file automation.c         \brief uHAB server automation
 */

#include "uhab.h"
#include "jscript.h"

TRACE_TAG(automation);
#if !ENABLE_TRACE_AUTOMATION
#include "trace_undef.h"
#endif

// Prototypes:
static void uhab_automation_cleanup(uhab_automation_t *au);


int uhab_automation_init(uhab_automation_t *au)
{
   DIR *d = NULL;
   struct dirent *dir;
   char path[255];

   os_memset(au, 0, sizeof(uhab_automation_t));

   LIST_STRUCT_INIT(au, scripts);

   // Create process mutex
   if ((au->mutex = osMutexCreate(NULL)) == NULL)
   {
      TRACE_ERROR("Create mutext failed");
      throw_exception(fail_mutex);
   }

   // List rules files
   if ((d = opendir(CFG_UHAB_RULES_CFG_DIR)) == NULL)
   {
      TRACE_ERROR("Can't open dir %s", CFG_UHAB_RULES_CFG_DIR);
      throw_exception(fail_opendir);
   }

   // Load rules config
   while ((dir = readdir(d)) != NULL)
   {
      if (strstr(dir->d_name, ".rules") != NULL)
      {
         snprintf(path, sizeof(path), "%s/%s", CFG_UHAB_RULES_CFG_DIR, dir->d_name);
         if (uhab_config_rules_load(path, au) != 0)
         {
            TRACE_ERROR("Load automation rules: %s", path);
            throw_exception(fail_readdir);
         }
      }
   }

   // Initialize javascript interpreter
   if (uhab_jscript_init(au) != 0)
   {
      TRACE_ERROR("Initialize javascript engine");
      throw_exception(fail_jscript_init);
   }

   // Clean not used resources
   uhab_automation_cleanup(au);
   closedir(d);

   au->initialized = 1;

   TRACE("Init");

   return 0;

fail_jscript_init:
fail_readdir:
   closedir(d);
fail_opendir:
   osMutexDelete(au->mutex);
fail_mutex:
   return -1;
}

int uhab_automation_deinit(uhab_automation_t *au)
{
   int res = 0;

   res += uhab_jscript_deinit();

   if (au->mutex != NULL)
      osMutexDelete(au->mutex);

   os_memset(au, 0, sizeof(uhab_automation_t));

   TRACE("Deinit");

   return res;
}

int uhab_automation_process_event(uhab_automation_t *au, uhab_rule_event_t event, uhab_item_t *item, uhab_item_state_t *newstate)
{
   int res = 0;
   uhab_rule_t *rule;

   if (!au->initialized)
   {
      TRACE_ERROR("Automation does not initialized");
      return -1;
   }

   osMutexWait(au->mutex, osWaitForever);

   // Update state property only
   res += uhab_jscript_update(item, newstate);

   // Execute rule by event type
   for (rule = list_head(item->automation.rules); rule != NULL; rule = list_item_next(rule))
   {
      if (rule->event == event || ((rule->event == UHAB_RULE_EVENT_CHANGED) && (event & UHAB_RULE_EVENT_CHANGED)))
      {
         // Eexecute rule function handler
         item->automation.event_pending = 1;
         res += uhab_jscript_execute(item, rule->jscript_function, newstate);
         item->automation.event_pending = 0;
      }
   }

   osMutexRelease(au->mutex);

   return res;
}

/** Free not used resources after automation was initialized */
static void uhab_automation_cleanup(uhab_automation_t *au)
{
   uhab_automation_script_t *script;

   // Free all scripts strings
   while ((script = list_pop(au->scripts)) != NULL)
   {
      if (script->body != NULL)
      {
         os_free(script->body);
         script->body = NULL;
      }

      os_free(script);
   }
}
