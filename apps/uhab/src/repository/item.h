/**
 * \file item.h         \brief uHAB repository item definition
 */

#ifndef __UHAB_ITEM_H
#define __UHAB_ITEM_H

#include "item_state.h"

// Forwarde decl.
struct uhab_rule;


/** Item types */
typedef enum
{
   UHAB_ITEM_TYPE_SYSTEM,   
   UHAB_ITEM_TYPE_SWITCH,
   UHAB_ITEM_TYPE_CONTACT,
   UHAB_ITEM_TYPE_NUMBER,
   UHAB_ITEM_TYPE_GROUP,
   UHAB_ITEM_TYPE_STRING,
   UHAB_ITEM_TYPE_DIMMER,
   UHAB_ITEM_TYPE_TIMER,
   UHAB_ITEM_TYPE_SCENE,
   UHAB_ITEM_TYPE_COLOR,
   UHAB_ITEM_TYPE_ROLLERSHUTTER,
   
} uhab_item_type_t;


/** Item stereotypes */
typedef enum
{
   UHAB_ITEM_STEREOTYPE_NONE,
   UHAB_ITEM_STEREOTYPE_LIST
   
} uhab_item_stereotype_t;


/** Item definition */
typedef struct uhab_item
{
   struct uhab_item *next;
   
   /** Current item state */
   uhab_item_state_t state;
   
   /** Item type */
   uhab_item_type_t type;
   
   /** Item stereotype */
   uhab_item_stereotype_t stereotype;
  
   /** Item name */
   const char *name;
   
   /** Item label */
   const char *label;
   
   /** Ico name */
   const char *icon_name;

   /** Additional tag type */
   const char *tag;
   
   /** Child items */
   LIST_STRUCT(child_items);
   
   /** Selected child item */
   struct uhab_child_item *active_child_item;

   
   /** Binding data */
   struct
   {
      /** Protocol binding */
      const struct uhab_protocol_binding *protocol;

      /** Binding configuration */
      char *config;
      
      /** Ptr to item used by protocol */
      void *protocol_item;
      
   } binding;
   
   
   /** BUS data */
   struct
   {
      /** Contact click measurement */
      struct
      {
         hal_time_t start_time;
         int count;
         osTimerId timer;
         
      } click;
      
      /** Last update time */
      hal_time_t update_time;
      
   } bus;
   
   
   /** Automation data */
   struct
   {
      /** Javascript object */
      uint64_t jsobject;

      /** Automation rules */
      LIST_STRUCT(rules);
      
      /** Event handler pending flag */
      uint8_t event_pending;
      
   } automation;
   
   
} uhab_item_t;


/** Child item */
typedef struct uhab_child_item
{
   struct uhab_child_item *next;
   
   /** Ptr to item link */
   uhab_item_t *item;
      
} uhab_child_item_t;



/** Alloc item */
uhab_item_t *uhab_item_alloc(uhab_item_type_t type);

/** Free item */
void uhab_item_free(uhab_item_t *item);

/** Add item automation rule */
int uhab_item_add_rule(uhab_item_t *item, struct uhab_rule *rule);


#endif // __UHAB_ITEM_H