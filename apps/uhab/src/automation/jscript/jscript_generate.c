/**
 * \file jscript_generate.c         \file Javascript generator from uHAB repository definition
 */
 
#include "uhab.h"

TRACE_GROUP(automation);
#if !ENABLE_TRACE_JSCRIPT
#include "trace_undef.h"
#endif

#define ACTION_TIMER_FMTNAME  "timer%p"

// Prototypes:
static int generate_rule_action(FILE *fs, uhab_rule_t *rule, uhab_rule_action_t *action);


/** Generate javascript functions */
int uhab_jscript_generate(uhab_automation_t *au, const char *jscript_filename)
{
   FILE *fs;
   uhab_item_t *item;
   uhab_rule_t *rule;
   uhab_rule_action_t *action, *prev_action;
   uint8_t timer_def;
   uhab_automation_script_t *script;
   char buf[255];
   
   // Create javascript rules file
   if ((fs = fopen(jscript_filename, "w")) == NULL)
   {
      TRACE_ERROR("Can't create javascript rules file", jscript_filename);
      throw_exception(fail_create);
   }
   
   TRACE("Generate jscript rules file: %s", jscript_filename);
   
   fprintf(fs, "//\n");
   fprintf(fs, "// Generated javascript items rules, don't edit this file !\n");
   fprintf(fs, "//\n\n");

   fprintf(fs, "// Commands \n");
   fprintf(fs, "var OFF=%.0f;\n", CFG_MAX_DOUBLE_VALUE + UHAB_ITEM_STATE_CMD_OFF);
   fprintf(fs, "var ON=%.0f;\n", CFG_MAX_DOUBLE_VALUE + UHAB_ITEM_STATE_CMD_ON);
   fprintf(fs, "var TOGGLE=%.0f;\n", CFG_MAX_DOUBLE_VALUE + UHAB_ITEM_STATE_CMD_TOGGLE);
   fprintf(fs, "var UP=%.0f;\n", CFG_MAX_DOUBLE_VALUE + UHAB_ITEM_STATE_CMD_UP);
   fprintf(fs, "var DOWN=%.0f;\n", CFG_MAX_DOUBLE_VALUE + UHAB_ITEM_STATE_CMD_DOWN);
   fprintf(fs, "var START=%.0f;\n", CFG_MAX_DOUBLE_VALUE + UHAB_ITEM_STATE_CMD_START);
   fprintf(fs, "var STOP=%.0f;\n", CFG_MAX_DOUBLE_VALUE + UHAB_ITEM_STATE_CMD_STOP);
   fprintf(fs, "var DIM_START=START;\n");
   fprintf(fs, "var DIM_STOP=STOP;\n");
   fprintf(fs, "\n");
   fprintf(fs, "// Timer types:\n");
   fprintf(fs, "var TIMER_ONCE=0\n");
   fprintf(fs, "var TIMER_PERIODIC=1\n");
   fprintf(fs, "\n");
   
   for (script = list_head(au->scripts); script != NULL; script = list_item_next(script))
   {
      fprintf(fs, "%s\n", script->body);
   }
   
   // Items
   for (item = list_head(repository.items); item != NULL; item = list_item_next(item))
   {
      // Rules
      for (rule = list_head(item->automation.rules); rule != NULL; rule = list_item_next(rule))
      {
         timer_def = 1;
         
         // Prepare actions
         for (action = list_head(rule->actions); action != NULL; action = list_item_next(action))
         {
            switch(action->type)
            {
               case UHAB_ACTION_DELAY:
               {
                  // Define timer only once for every rule
                  if (timer_def)
                  {
                     fprintf(fs, "var " ACTION_TIMER_FMTNAME "=null;\n\n", rule);
                     timer_def = 0;
                  }
                  
                  fprintf(fs, "function " ACTION_TIMER_FMTNAME "_cb() {\n", action);
                  fprintf(fs, ACTION_TIMER_FMTNAME ".destroy();\n", rule);
                  fprintf(fs, ACTION_TIMER_FMTNAME "=null;\n", rule);
                  
                  // Ale inner actions until next delay
                  for (prev_action = action, action = list_item_next(action); action != NULL; action = list_item_next(action))
                  {
                     generate_rule_action(fs, rule, action);
                     
                     if (action->type == UHAB_ACTION_DELAY)
                     {
                        action = prev_action;
                        break;
                     }
                     
                     prev_action = action;
                  }
                  
                  fprintf(fs, "}\n\n");
               }
               break;
               
               default:
                  break;
            }
         }

         // Generate javascript function
         snprintf(buf, sizeof(buf), "%s_%s_event", item->name, rule->evtdef->name);
         if ((rule->jscript_function = os_strdup(buf)) == NULL)
              throw_exception(fail_generate);

         fprintf(fs, "function %s() {\n", buf);    
         
         // Post actions
         for (action = list_head(rule->actions); action != NULL; action = list_item_next(action))
         {
            generate_rule_action(fs, rule, action);
            
            // Gener only first delay - next commands are pregenerated
            if (action->type == UHAB_ACTION_DELAY)
               break;
         }
         
         fprintf(fs, "}\n\n");
      }
   }
   
   fclose(fs);
   
   return 0;

fail_generate:
   fclose(fs);
fail_create:
   return -1;
}


static int generate_rule_action(FILE *fs, uhab_rule_t *rule, uhab_rule_action_t *action)
{
   if (action->condition != NULL)
      fprintf(fs, "if(%s) {\n", action->condition);

   switch(action->type)
   {
      case UHAB_ACTION_EXEC_JSCRIPT:
         fprintf(fs, "%s\n", action->param);
         break;
      
      case UHAB_ACTION_SEND_COMMAND:
         fprintf(fs, "%s.send_command(%s);\n", action->item->name, action->param);
         break;
      
      case UHAB_ACTION_DELAY:
      {
         ASSERT(action->param != NULL);

         fprintf(fs, "if (" ACTION_TIMER_FMTNAME " != null) " ACTION_TIMER_FMTNAME ".destroy();\n", rule, rule);
         fprintf(fs, ACTION_TIMER_FMTNAME " = timer_create(TIMER_ONCE, " ACTION_TIMER_FMTNAME "_cb);\n", rule, action);
         fprintf(fs, ACTION_TIMER_FMTNAME ".start(%d);\n", rule, atoi(action->param));         
      }
      break;
   }

   if (action->condition != NULL)
      fprintf(fs, "}\n");

   return 0;
}
