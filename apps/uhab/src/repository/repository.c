/**
 * \file repository.c         \brief uHAB repository
 */

#include "uhab.h"

TRACE_TAG(repository);
#if !ENABLE_TRACE_REPOSITORY
#include "trace_undef.h"
#endif


/** Open repository */
int uhab_repository_init(uhab_repository_t *repo)
{
   char *pp;
   uhab_item_t *item;
   DIR *d = NULL;
   struct dirent *dir;
   char txt[512];

   memset(repo, 0, sizeof(uhab_repository_t));
   LIST_STRUCT_INIT(repo, items);

   if ((repo->items_mutex = osMutexCreate(NULL)) == NULL)
      throw_exception(fail);

   // Add default system items
   if (repository_add_new_item(repo, CFG_UHAB_SYSTEM_ITEM_NAME, UHAB_ITEM_TYPE_SYSTEM) == NULL)
      throw_exception(fail);

   // List files
   if ((d = opendir(CFG_UHAB_ITEMS_CFG_DIR)) == NULL)
   {
      TRACE_ERROR("Can't open dir %s", CFG_UHAB_ITEMS_CFG_DIR);
      return -1;
   }

   // Load config
   while ((dir = readdir(d)) != NULL)
   {
      if (strstr(dir->d_name, ".items") != NULL)
      {
         snprintf(txt, sizeof(txt), "%s/%s", CFG_UHAB_ITEMS_CFG_DIR, dir->d_name);
         if (uhab_config_items_load(txt, repo) != 0)
         {
            TRACE_ERROR("Load items file %s", txt);
            throw_exception(fail);
         }
      }
   }

   for (item = list_head(repo->items); item != NULL; item = list_item_next(item))
   {
      if (item->binding.config != NULL)
      {
         // Get binding name from config
         if ((pp = strchr(item->binding.config, '=')) == NULL)
         {
            TRACE_ERROR("Not specified binding name in config '%s'", item->binding.config);
            throw_exception(fail);
         }

         if (pp - item->binding.config > sizeof(txt))
         {
            TRACE_ERROR("Binding name length overflow");
            throw_exception(fail);
         }

         strncpy(txt, item->binding.config, pp - item->binding.config);
         txt[pp - item->binding.config] = '\0';

         if ((item->binding.protocol = uhab_binding_get_by_name(txt)) == NULL)
         {
            TRACE_ERROR("Binding '%s' not defined", txt);
            throw_exception(fail);
         }

         // Configure bindings
         if (item->binding.protocol->configure(item, item->binding.config) != 0)
         {
            TRACE_ERROR("Configure item: %s binding to %s", item->name, item->binding.protocol->name);
            throw_exception(fail);
         }
      }
   }

   closedir(d);
   TRACE("Repository items count: %d", uhab_repository_get_items_count(repo));

   return 0;

fail:
   if (d != NULL)
      closedir(d);
   return -1;
}

/** Close repository */
int uhab_repository_deinit(uhab_repository_t *repo)
{
   if (repo->items_mutex != NULL)
   {
      osMutexDelete(repo->items_mutex);
      repo->items_mutex = NULL;
   }

   TRACE("Repository closed");
   return 0;
}

/** Get item by name from repository */
uhab_item_t *uhab_repository_get_item(uhab_repository_t *repo, const char *name)
{
   uhab_item_t *item;

   for (item = list_head(repo->items); item != NULL; item = list_item_next(item))
   {
      if (!strcmp(item->name, name))
         break;
   }

   return item;
}

/** Get owned child item by name */
uhab_item_t *uhab_repository_get_child_item(uhab_item_t *parent, const char *name)
{
   uhab_child_item_t *li;

   for (li = list_head(parent->child_items); li != NULL; li = list_item_next(li))
   {
      if (!strcmp(li->item->name, name))
         break;
   }

   return (li != NULL) ? li->item : NULL;
}


/** Get repository items count */
int uhab_repository_get_items_count(uhab_repository_t *repo)
{
   int count = 0;
   uhab_item_t *item;

   for (item = list_head(repo->items); item != NULL; item = list_item_next(item), count++);

   return count;
}

/** Add item to repository */
int uhab_repository_add_item(uhab_repository_t *repo, uhab_item_t *item)
{
   ASSERT(item != NULL);
   ASSERT(item->name != NULL);

   if (uhab_repository_get_item(repo, item->name) != NULL)
   {
      TRACE_ERROR("Item '%s' already exists in the repository", item->name);
      return -1;
   }

   list_add(repo->items, item);

   return 0;
}

/** Add new item to repository */
uhab_item_t * repository_add_new_item(uhab_repository_t *repo, const char *name, uhab_item_type_t type)
{
   uhab_item_t *item = NULL;

   if ((item = uhab_item_alloc(type)) == NULL)
   {
      TRACE_ERROR("Alloc item");
      throw_exception(fail);
   }

   if ((item->name = strdup(name)) == NULL)
      throw_exception(fail);

   if (uhab_repository_add_item(repo, item) != 0)
   {
      TRACE_ERROR("Add item");
      throw_exception(fail);
   }

   return item;

fail:
   if (item != NULL)
      uhab_item_free(item);

   return NULL;
}

/** Add child item to parent item */
uhab_child_item_t *uhab_repository_add_child_item(uhab_item_t *parent, uhab_item_t *item)
{
   uhab_child_item_t *child_item;

   ASSERT(item != NULL);
   ASSERT(item->name != NULL);

   if (uhab_repository_get_child_item(parent, item->name) != NULL)
   {
      TRACE_ERROR("Item: %s already exists in %s", item->name, parent->name);
      return NULL;
   }

   if ((child_item = os_malloc(sizeof(uhab_child_item_t))) == NULL)
   {
      TRACE_ERROR("Alloc child item: %s", item->name);
      return NULL;
   }

   memset(child_item, 0, sizeof(uhab_child_item_t));
   child_item->item = item;
   list_add(parent->child_items, child_item);

   return child_item;
}
