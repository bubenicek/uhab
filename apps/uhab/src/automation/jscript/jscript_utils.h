
#ifndef __JSCRIPT_UTILS_H
#define __JSCRIPT_UTILS_H

int itemstate_to_jsvalue(struct v7 *v7, const uhab_item_state_t *state, v7_val_t *value);

int jsvalue_to_itemstate(struct v7 *v7, v7_val_t *value, uhab_item_state_t *state);

#endif // __JSCRIPT_UTILS_H