/**
 * \file scenes_config.c        \brief Scenes configuration
 */
 
#include "uhab.h"
#include "roxml.h"

TRACE_TAG(cfg_scenes);
#if !ENABLE_TRACE_CONF
#include "trace_undef.h"
#endif

/** Load scenes */
int uhab_config_scenes_load(uhab_repository_t *repo)
{
   int fd, argc, ix;
   node_t *doc, *root, *scene_node, *item_node, *bank_node;
   uhab_item_t *scene, *item, *bank;
   uhab_child_item_t *child_item;   
   const char *value;
   char *argv[CFG_MAXNUM_ITEMS_GROUPS];   
   char path[255];
   
   // Open config file
   snprintf(path, sizeof(path), CFG_UHAB_SCENES_CFG_FILENAME, repo->name);
   if ((fd = open(path, O_RDONLY)) < 0)
   {
      TRACE_ERROR("Can't open items cfg file %s", path);
      throw_exception(fail_open);
   }
   
   TRACE("Scenes cfg: %s open", path);

   if ((doc = roxml_load_fd(fd)) == NULL)
   {
      TRACE_ERROR("Load scenes xml failed");
      throw_exception(fail_load);
   }
   
   if ((root = roxml_get_chld(doc, "scenes", 0)) == NULL)
   {
      TRACE_ERROR("Scenes not found");
      throw_exception(fail_parse);
   }
   
   // Load all scenes
   for (scene_node = roxml_get_chld(root, "scene", 0); scene_node != NULL; scene_node = roxml_get_next_sibling(scene_node))           
   {
      if (strcmp(roxml_get_name(scene_node, NULL, 0), "scene") != 0)
         continue;	     

      // Alloc new scene
      if ((scene = uhab_item_alloc(UHAB_ITEM_TYPE_SCENE)) == NULL)
         throw_exception(fail_parse);

      // Get scene name
      if ((value = roxml_get_attr_value(scene_node, "name")) == NULL)
      {
         TRACE_ERROR("Expected scene name");
         throw_exception(fail_parse);
      }
      if ((scene->name = os_strdup(value)) == NULL)
         throw_exception(fail_parse);

      // Get scene label
      if ((value = roxml_get_attr_value(scene_node, "label")) == NULL)
      {
         TRACE_ERROR("Expected scene label");
         throw_exception(fail_parse);
      }
      if ((scene->label = os_strdup(value)) == NULL)
         throw_exception(fail_parse);

      // Add scene to repository
      if (uhab_repository_add_item(repo, scene) != 0)
      {
         TRACE_ERROR("Add scene to repo");
         throw_exception(fail_parse);
      }
      
      TRACE("Scene: %s", scene->name);

      // Add all items to scene
      for (item_node = roxml_get_chld(scene_node, "item", 0); item_node != NULL; item_node = roxml_get_next_sibling(item_node))           
      {
         if (strcmp(roxml_get_name(item_node, NULL, 0), "item") != 0)
            continue;	     

         if ((value = roxml_get_attr_value(item_node, "name")) == NULL)
         {
            TRACE_ERROR("Expected item name");
            throw_exception(fail_parse);
         }
         
         // Find item
         if ((item = uhab_repository_get_item(repo, value)) == NULL)
         {
            TRACE_ERROR("Item: %s of scene: %s not exists", value, scene->name);
            throw_exception(fail_parse);
         }

         // Get item state
         if ((value = roxml_get_attr_value(item_node, "state")) == NULL)
         {
            TRACE_ERROR("Expected state of item: %s scene: %s", item->name, scene->name);
            throw_exception(fail_parse);
         }
    
         if ((child_item = uhab_repository_add_child_item(scene, item)) == NULL)
         {
            TRACE_ERROR("Add item: %s to scene: %s", item->name, scene->name);
            throw_exception(fail_parse);
         }

         // Set item scene state
         if ((child_item->scene_state = os_strdup(value)) == NULL)
            throw_exception(fail_parse);
    
         TRACE("   Item: %s   state: %s", child_item->item->name, child_item->scene_state);
      }
   }

   // Add all banks
   for (bank_node = roxml_get_chld(root, "scene_bank", 0); bank_node != NULL; bank_node = roxml_get_next_sibling(bank_node))           
   {
      if (strcmp(roxml_get_name(bank_node, NULL, 0), "scene_bank") != 0)
         continue;	     
      
      // Alloc new bank
      if ((bank = uhab_item_alloc(UHAB_ITEM_TYPE_SCENE_BANK)) == NULL)
         throw_exception(fail_parse);

      if ((value = roxml_get_attr_value(bank_node, "name")) == NULL)
      {
         TRACE_ERROR("Expected item name");
         throw_exception(fail_parse);
      }
      if ((bank->name = os_strdup(value)) == NULL)
         throw_exception(fail_parse);
         
      if ((value = roxml_get_attr_value(bank_node, "label")) != NULL)
      {
         if ((bank->label = os_strdup(value)) == NULL)
            throw_exception(fail_parse);
      }

      // Add scene bank to repository
      if (uhab_repository_add_item(repo, bank) != 0)
      {
         TRACE_ERROR("Add scene bank: %s to repo", bank->name);
         throw_exception(fail_parse);
      }
      
      TRACE("Scenes bank: %s", bank->name);
      
      argc = split_line(roxml_get_attr_value(bank_node, "scene"), ',', argv, CFG_MAXNUM_ITEMS_GROUPS);
      if (argc > 0)
      {
         for (ix = 0; ix < argc; ix++)
         {
            str_remove(argv[ix], ' ');

            // Find scene by name
            scene = uhab_repository_get_item(repo, argv[ix]);
            if (scene == NULL)
            {
               TRACE_ERROR("Scene: %s of bank: %s not defined", argv[ix], bank->name);
               throw_exception(fail_parse);
            }
            
            // Check scene type
            if (scene->type != UHAB_ITEM_TYPE_SCENE)
            {
               TRACE_ERROR("Item '%s' specified as scene is not scene type", scene->name);
               throw_exception(fail_parse);
            }

            // Add scene to bank
            if (uhab_repository_add_child_item(bank, scene) == NULL)
            {
               TRACE_ERROR("Add scene: %s to bank: %s", scene->name, bank->name);
               throw_exception(fail_parse);
            }
            
            TRACE("   %s", scene->name);
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
