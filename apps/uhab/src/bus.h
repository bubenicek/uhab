/**
 * \file bus.h       \brief uHAB virtual BUS
 */

#ifndef __UHAB_BUS_H
#define __UHAB_BUS_H

#include "uiprovider/widget.h"


/** Waitstate object */
typedef struct uhab_bus_waitstate
{
   struct uhab_bus_waitstate *next;
   uint8_t active;
   osSemaphoreId sem;
   uhab_sitemap_widget_t *parent_widget;

} uhab_bus_waitstate_t;


/** BUS event */
typedef struct
{
   uhab_item_t *item;
   uhab_item_state_t state;

} uhab_bus_event_t;


/** Initialize event bus */
int uhab_bus_init(void);

/** Deinitialize event bus */
int uhab_bus_deinit(void);

/** Send new state to item */
int uhab_bus_send(const uhab_item_t *item, const uhab_item_state_t *state);

/** Update item state without sending to binding */
int uhab_bus_update(const uhab_item_t *item, const uhab_item_state_t *state);

/** Wait for any changes */
int uhab_bus_waitfor_changes(uhab_sitemap_widget_t *parent_widget);


#endif // __UHAB_EVENT_BUS_H
