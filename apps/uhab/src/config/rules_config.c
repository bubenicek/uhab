/**
 * \file rules_config.c         \brief Automaton rules configuration
 */

#include "uhab.h"
#include "roxml.h"

TRACE_TAG(cfg_rules);
#if !ENABLE_TRACE_CONFIG
#include "trace_undef.h"
#endif


// Prototypes:
static void encode_script_string(char *str);
static int parse_script(uhab_automation_t *au, node_t *script_node);


/** Load rules configuration */
int uhab_config_rules_load(const char *path, uhab_automation_t *au)
{
   int fd;
   node_t *doc_node, *root_node, *node, *rule_node, *action_node;
   uhab_item_t *item;
   uhab_rule_t *rule;
   uhab_rule_action_t *action;
   char *value;

   // Open config file
   if ((fd = open(path, O_RDONLY)) < 0)
   {
      TRACE_ERROR("Can't open items cfg file %s", path);
      throw_exception(fail_open);
   }
   TRACE("Open rules cfg: %s", path);

   if ((doc_node = roxml_load_fd(fd)) == NULL)
   {
      TRACE_ERROR("Load rules xml failed");
      throw_exception(fail_load);
   }

   if ((root_node = roxml_get_chld(doc_node, "rules", 0)) == NULL)
   {
      TRACE_ERROR("Rules not found");
      throw_exception(fail_parse);
   }

   for (node = roxml_get_chld(root_node, NULL, 0); node != NULL; node = roxml_get_next_sibling(node))
   {
      if (!strcasecmp(roxml_get_name(node, NULL, 0), "script"))
      {
         if (parse_script(au, node) != 0)
         {
            TRACE_ERROR("Parse script failed");
            throw_exception(fail_parse);
         }
      }
      else
      {

         if ((item = uhab_repository_get_item(&repository, roxml_get_name(node, NULL, 0))) == NULL)
         {
            TRACE_ERROR("Item '%s' not defined", roxml_get_name(node, NULL, 0));
            throw_exception(fail_parse);
         }

         TRACE("Item: %s", item->name);

         // Rules
         for (rule_node = roxml_get_chld(node, "rule", 0); rule_node != NULL; rule_node = roxml_get_next_sibling(rule_node))
         {
            if ((rule = uhab_rule_alloc()) == NULL)
            {
               TRACE_ERROR("Alloc rule");
               throw_exception(fail_parse);
            }

            // Rule name
            if ((value = roxml_get_attr_value(rule_node, "name")) != NULL)
            {
               if ((rule->name = os_strdup(value)) == NULL)
                  throw_exception(fail_parse);
            }

            // Rule event
            if ((value = roxml_get_attr_value(rule_node, "event")) == NULL)
            {
               TRACE_ERROR("Undefined rule '%s' event", rule->name);
               throw_exception(fail_parse);
            }
            if ((rule->evtdef = uhab_rule_get_definition(value)) == NULL)
            {
               TRACE_ERROR("Not supported rule '%s' event: %s", rule->name, value);
               throw_exception(fail_parse);
            }
            rule->event = rule->evtdef->type;
            TRACE("   Rule: %s  Event: %s", rule->name, rule->evtdef->name);

            // Add to rules list
            if (uhab_item_add_rule(item, rule) != 0)
            {
               TRACE_ERROR("Add rule to item: %s", item->name);
               throw_exception(fail_parse);
            }

            // Actions
            for (action_node = roxml_get_chld(rule_node, "action", 0); action_node != NULL; action_node = roxml_get_next_sibling(action_node))
            {
               if ((action = uhab_rule_action_alloc()) == NULL)
               {
                  TRACE_ERROR("Alloc action");
                  throw_exception(fail_parse);
               }

               // Action type
               if ((value = roxml_get_attr_value(action_node, "type")) != NULL)
               {
                  if ((action->actdef = uhab_rule_action_get_definition(value)) == NULL)
                  {
                     TRACE_ERROR("Not supported action type: %s", roxml_get_attr_value(action_node, "type"));
                     throw_exception(fail_parse);
                  }
                  action->type = action->actdef->type;
               }
               else
               {
                  TRACE_ERROR("Not defined action type of rule: %s", rule->name);
                  throw_exception(fail_parse);
               }

               // Action condition
               if ((value = roxml_get_attr_value(action_node, "condition")) != NULL)
               {
                  if ((action->condition = os_strdup(value)) == NULL)
                     throw_exception(fail_parse);
               }

               switch(action->type)
               {
                  case UHAB_ACTION_EXEC_JSCRIPT:
                  {
                     if ((value = roxml_get_content(action_node, NULL, 0, NULL)) != NULL)
                     {
                        // Replace tab and remove spaces
                        encode_script_string(value);

                        if ((action->param = os_strdup(value)) == NULL)
                           throw_exception(fail_parse);
                     }
                     else
                     {
                        TRACE_ERROR("Not defined action javascript body of rule: %s", rule->name);
                        throw_exception(fail_parse);
                     }
                  }
                  break;

                  case UHAB_ACTION_SEND_COMMAND:
                  {
                     // Action item
                     if ((value = roxml_get_attr_value(action_node, "item")) != NULL)
                     {
                        if ((action->item = uhab_repository_get_item(&repository, value)) == NULL)
                        {
                           TRACE_ERROR("Undefined action item: %s of rule: %s", value, rule->name);
                           throw_exception(fail_parse);
                        }
                     }
                     else
                     {
                        TRACE_ERROR("Not defined action item of rule: %s", rule->name);
                        throw_exception(fail_parse);
                     }

                     // Action param
                     if ((value = roxml_get_attr_value(action_node, "param")) != NULL)
                     {
                        if ((action->param = os_strdup(value)) == NULL)
                           throw_exception(fail_parse);
                     }
                     else
                     {
                        TRACE_ERROR("Not defined action param of rule: %s", rule->name);
                        throw_exception(fail_parse);
                     }

                     TRACE("      '%s'  '%s'  '%s'", action->actdef->name, action->item->name, action->param);
                  }
                  break;

                  case UHAB_ACTION_DELAY:
                  {
                     // Action param
                     if ((value = roxml_get_attr_value(action_node, "param")) != NULL)
                     {
                        if ((action->param = os_strdup(value)) == NULL)
                           throw_exception(fail_parse);
                     }
                     else
                     {
                        TRACE_ERROR("Not defined action param of rule: %s", rule->name);
                        throw_exception(fail_parse);
                     }

                     TRACE("      '%s'   '%s'", action->actdef->name, action->param);
                  }
                  break;
               }

               // Add to actions list
               list_add(rule->actions, action);
            }
         }
      }
   }

   roxml_release(RELEASE_ALL);
   roxml_close(doc_node);
   close(fd);

   return 0;

fail_parse:
   roxml_release(RELEASE_ALL);
   roxml_close(doc_node);
fail_load:
   close(fd);
fail_open:
   return -1;
}


static void encode_script_string(char *str)
{
   str_replace(str, '\t', ' ');
   str_replace_str(str, "&lt;", "<");
   str_replace_str(str, "&gt;", "<");
   str_replace_str(str, "&amp;", "&");
   str_replace_str(str, "&quot;", "\"");
   str_replace_str(str, "&apos;", "'");
}


static int parse_script(uhab_automation_t *au, node_t *script_node)
{
   int fd = -1;
   struct stat st;
   char path[255];
   char *value;
   uhab_automation_script_t *script = NULL;

   if ((script = os_malloc(sizeof(uhab_automation_script_t))) == NULL)
   {
      TRACE_ERROR("Alloc script");
      throw_exception(fail);
   }

   if ((value = roxml_get_attr_value(script_node, "src")) != NULL)
   {
      // Include from file

      snprintf(path, sizeof(path), "%s/%s", CFG_UHAB_RULES_CFG_DIR, value);
      if ((fd = open(path, O_RDONLY)) < 0)
      {
         TRACE_ERROR("Can't open script file %s", path);
         throw_exception(fail);
      }

      if (fstat(fd, &st) < 0)
      {
         TRACE_ERROR("Can't read script %s size", path);
         throw_exception(fail);
      }

      if (st.st_size > 0)
      {
         if ((script->body = os_malloc(st.st_size + 1)) == NULL)
         {
            TRACE("Alloc script %s body", path);
            throw_exception(fail);
         }

         if (read(fd, script->body, st.st_size) != st.st_size)
         {
            TRACE_ERROR("Read script %s body", path);
            throw_exception(fail);
         }

         script->body[st.st_size] = '\0';
         list_add(au->scripts, script);

         TRACE("Include script %s  size: %d", path, st.st_size);
      }

      close(fd);
   }
   else
   {
      if ((value = roxml_get_content(script_node, NULL, 0, NULL)) != NULL)
      {
         // Replace tab and remove spaces
         encode_script_string(value);

         if ((script->body = os_strdup(value)) == NULL)
            throw_exception(fail);

         list_add(au->scripts, script);
      }
   }

   return 0;

fail:
   if (fd != -1)
      close(fd);

   if (script != NULL)
      os_free(script);
   return -1;
}
