/**
 * \file items_confif.c         \brief Items configuration
 */

#include "uhab.h"
#include "roxml.h"

TRACE_TAG(cfg_items);
#if !ENABLE_TRACE_CONFIG
#include "trace_undef.h"
#endif

/** XML element definition */
typedef struct
{
   char *name;
   uhab_item_type_t type;
   uhab_item_state_type_t state_type;
   
} uhab_item_xml_element_t;


/** UAHB items XML elements */
static const uhab_item_xml_element_t uhab_item_elements [] = 
{
   {"group", UHAB_ITEM_TYPE_GROUP, UHAB_ITEM_STATE_TYPE_CMD},
   {"switch", UHAB_ITEM_TYPE_SWITCH, UHAB_ITEM_STATE_TYPE_CMD},
   {"contact", UHAB_ITEM_TYPE_CONTACT, UHAB_ITEM_STATE_TYPE_CMD},
   {"scene", UHAB_ITEM_TYPE_SCENE, UHAB_ITEM_STATE_TYPE_CMD},
   {"rollershutter", UHAB_ITEM_TYPE_ROLLERSHUTTER, UHAB_ITEM_STATE_TYPE_CMD},
   {"timer", UHAB_ITEM_TYPE_TIMER, UHAB_ITEM_STATE_TYPE_NUMBER},
   {"number", UHAB_ITEM_TYPE_NUMBER, UHAB_ITEM_STATE_TYPE_NUMBER},
   {"dimmer", UHAB_ITEM_TYPE_DIMMER, UHAB_ITEM_STATE_TYPE_NUMBER},
   {"color", UHAB_ITEM_TYPE_COLOR, UHAB_ITEM_STATE_TYPE_NUMBER},
   {"string", UHAB_ITEM_TYPE_STRING, UHAB_ITEM_STATE_TYPE_STRING},
   {NULL},
};


/** Load items */
int uhab_config_items_load(const char *path, uhab_repository_t *repo)
{
   int fd;
   uhab_item_t *item;
   node_t *doc, *root, *node;
   const char *value;   
   const uhab_item_xml_element_t *elem;
   
   // Open config file
   if ((fd = open(path, O_RDONLY)) < 0)
   {
      TRACE_ERROR("Can't open items cfg file %s", path);
      throw_exception(fail_open);
   }
   
   TRACE("Items cfg: %s open", path);

   if ((doc = roxml_load_fd(fd)) == NULL)
   {
      TRACE_ERROR("Load items xml failed");
      throw_exception(fail_load);
   }
   
   if ((root = roxml_get_chld(doc, "items", 0)) == NULL)
   {
      TRACE_ERROR("Items not found");
      throw_exception(fail_parse);
   }

   // Parse all defined elements
   for (elem = &uhab_item_elements[0]; elem->name != NULL; elem++)
   {
      for (node = roxml_get_chld(root, elem->name, 0); node != NULL; node = roxml_get_next_sibling(node))           
      {
         if (strcmp(roxml_get_name(node, NULL, 0), elem->name) != 0)
            continue;
         
         if ((item = uhab_item_alloc(elem->type)) == NULL)
         {
            TRACE_ERROR("Alloc item: %s", elem->name);
            throw_exception(fail_parse);
         }
         item->state.type = elem->state_type;
         
         // Get stereotype
         if ((value = roxml_get_attr_value(node, "stereotype")) != NULL)
         {
            if (!strcasecmp(value, "list"))
               item->stereotype = UHAB_ITEM_STEREOTYPE_LIST;
            else
            {
               TRACE_ERROR("Item: %s has defined not supported stereotype: %s", item->name, value);
               throw_exception(fail_parse);
            }
         }
            
         // Get name
         if ((value = roxml_get_attr_value(node, "name")) == NULL)
         {
            TRACE_ERROR("Expected item: %s name", elem->name);
            throw_exception(fail_parse);
         }
         if ((item->name = os_strdup(value)) == NULL)
            throw_exception(fail_parse);

         // Get label
         if ((value = roxml_get_attr_value(node, "label")) != NULL)
         {
            if ((item->label = os_strdup(value)) == NULL)
               throw_exception(fail_parse);
         }
         
         // Get TAG
         if ((value = roxml_get_attr_value(node, "tag")) != NULL)
         {
            if ((item->tag = os_strdup(value)) == NULL)
               throw_exception(fail_parse);
         }

         // Get binding
         if ((value = roxml_get_attr_value(node, "binding")) != NULL)
         {
            if ((item->binding.config = os_strdup(value)) == NULL)
               throw_exception(fail_parse);
         }

         // State || value
         if ((value = roxml_get_attr_value(node, "state")) != NULL ||
             (value = roxml_get_attr_value(node, "value")))
         {
            if (uhab_item_state_set_value(&item->state, value) != 0)
            {
               TRACE_ERROR("Set item: %s state/value failed", item->name);
               throw_exception(fail_parse);
            }
         }

         // Get group
         if ((value = roxml_get_attr_value(node, "group")) != NULL)
         {
            int argc, ix;
            char *argv[CFG_MAXNUM_ITEMS_GROUPS];
            uhab_item_t *grp = NULL;
           
            // Add item to defined groups
            argc = split_line((char *)value, ',', argv, CFG_MAXNUM_ITEMS_GROUPS);
            if (argc > 0)
            {
               for (ix = 0; ix < argc; ix++)
               {
                  str_remove(argv[ix], ' ');

                  // Get group by name
                  grp = uhab_repository_get_item(repo, argv[ix]);
                  if (grp == NULL)
                  {
                     TRACE_ERROR("Group: %s of item: %s not defined", argv[ix], item->name);
                     throw_exception(fail_parse);
                  }
                  
                  // Test group type
                  if (grp->type != UHAB_ITEM_TYPE_GROUP)
                  {
                     TRACE_ERROR("Item '%s' specified as group is not group type", grp->name);
                     throw_exception(fail_parse);
                  }
                  
                  // Add item to group
                  if (uhab_repository_add_child_item(grp, item) == NULL)
                  {
                     TRACE_ERROR("Add item: %s to group: %s", item->name, grp->name);
                     throw_exception(fail_parse);
                  }
               }
               
               if (grp != NULL)
               {
                  // Set active child item if exists
                  grp->active_child_item = list_head(grp->child_items);
               }
            }
         }            
         
         TRACE("   Item: '%s'  '%s'  '%s'", item->name, elem->name, item->binding.config);     
      
         // Add to repository
         if (uhab_repository_add_item(repo, item) != 0)
         {
            TRACE_ERROR("Add item: %s to repository", item->name);
            throw_exception(fail_parse);
         }
         
         
         
      }
   }

	roxml_release(RELEASE_ALL);
	roxml_close(doc);
	close(fd);
   
   TRACE("Parse done");

   return 0;

fail_parse:
	roxml_release(RELEASE_ALL);
	roxml_close(doc);
fail_load:	
	close(fd);
fail_open:   
   return -1;
}
