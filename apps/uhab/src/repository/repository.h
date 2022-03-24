/**
 * \file repository.h         \brief uHAB repository
 */

#ifndef __UHAB_REPOSITORY_H
#define __UHAB_REPOSITORY_H

#include "item.h" 


/** UHAB repository */
typedef struct
{  
   /** Items list */
   LIST_STRUCT(items);
   
   /** Access items mutex */
   osMutexId items_mutex;
   
} uhab_repository_t;

/** Open repository */
int uhab_repository_init(uhab_repository_t *repo);

/** Close repository */
int uhab_repository_deinit(uhab_repository_t *repo);

/** Get item by name from repository */
uhab_item_t *uhab_repository_get_item(uhab_repository_t *repo, const char *name);

/** Get items list pointer */
#define uhab_repository_get_items(_repo) ((_repo)->items)

/** Get repository items count */
int uhab_repository_get_items_count(uhab_repository_t *repo);

/** Get owned child item by name */
uhab_item_t *uhab_repository_get_child_item(uhab_item_t *parent, const char *name);

/** Add item to repository */
int uhab_repository_add_item(uhab_repository_t *repo, uhab_item_t *item);

/** Add new item to repository */
uhab_item_t *repository_add_new_item(uhab_repository_t *repo, const char *name, uhab_item_type_t type);

/** Add child item to parent item */
uhab_child_item_t *uhab_repository_add_child_item(uhab_item_t *parent, uhab_item_t *item);

#endif // __UHAB_REPOSITORY_H