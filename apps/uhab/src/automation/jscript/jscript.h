/**
 * \file jscript.h         \brief Automation javascript interpreter
 *
 * Supported API:
 * ==============
 *
 * Item API:
 * ---------
 *
 *
 *
 *
 *
 *
 *
 *
 * Timer API:
 * ----------
 *
 * TIMER_ONCE     = 0       // one-shot timer
 * TIMER_PERIODIC = 1       // repeating timer
 *
 * // Create timer
 * timer1 = timer_create(1, timer1_cb);
 *
 * // Destroy timer
 * timer1.destroy();
 *
 * // Start timer with defined period in ms
 * timer1.start(4000);
 *
 * // Stop timer
 * timer1.stop();
 *

 */

#ifndef __JSCRIPT_H
#define __JSCRIPT_H

#include "v7.h"
#include "jscript_items.h"
#include "jscript_timer.h"
#include "jscript_generate.h"
#include "jscript_utils.h"

/** Initialize javascript */
int uhab_jscript_init(uhab_automation_t *au);

/** Deinitialize javascript */
int uhab_jscript_deinit(void);

/** Execute javascript rule function */
int uhab_jscript_execute(uhab_item_t *item, const char *funcname, uhab_item_state_t *newstate);

/** Update javascript state property */
int uhab_jscript_update(uhab_item_t *item, uhab_item_state_t *newstate);

/** Lock access to jscript */
void uhab_jscript_lock(void);

/** Unlock access to jscript */
void uhab_jscript_unlock(void);

#endif // __JSCRIPT_H