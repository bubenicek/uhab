
#ifndef __ITEM_STATE_H
#define __ITEM_STATE_H

#define UHAB_ITEM_STATE_INIT(_type) {.type = _type}
#define UHAB_ITEM_STATE_INIT_CMD(_cmd) {.type = UHAB_ITEM_STATE_TYPE_CMD, .value.cmd = _cmd}
#define UHAB_ITEM_STATE_INIT_NUMBER(_num) {.type = UHAB_ITEM_STATE_TYPE_NUMBER, .value.number = _num}
#define UAHB_ITEM_STATE_INIT_STRING_REF(_str) {.type = UHAB_ITEM_STATE_TYPE_STRING, .value.str = (char *)_str}


/** Item state types */
typedef enum
{
   UHAB_ITEM_STATE_TYPE_NONE,
   UHAB_ITEM_STATE_TYPE_CMD,
   UHAB_ITEM_STATE_TYPE_NUMBER,
   UHAB_ITEM_STATE_TYPE_STRING,
      
} uhab_item_state_type_t;


/** Command state values */
typedef enum
{
   UHAB_ITEM_STATE_CMD_OFF,
   UHAB_ITEM_STATE_CMD_ON,
   UHAB_ITEM_STATE_CMD_TOGGLE,
   UHAB_ITEM_STATE_CMD_UP,
   UHAB_ITEM_STATE_CMD_DOWN,
   UHAB_ITEM_STATE_CMD_START,
   UHAB_ITEM_STATE_CMD_STOP,
      
   UHAB_ITEM_STATE_CMD_END
   
} uhab_item_state_cmd_t;


/** Item state */
typedef struct uhab_item_state
{
   /** Item state type */
   uhab_item_state_type_t type;
   
   /** State value */
   union
   {
      uhab_item_state_cmd_t cmd;         // Command value
      double number;                     // Number
      char *str;                         // String 
      
   } value;
   
} uhab_item_state_t;


/** Translate string to command */
int uhab_item_state_str2command(const char *str, uhab_item_state_cmd_t *cmd);

/** Get item state value as string */
const char *uhab_item_state_get_value_fmt(const uhab_item_state_t *state, const char *fmt, char *buf, int bufsize);
const char *uhab_item_state_get_value(const uhab_item_state_t *state, char *buf, int bufsize);

/** Set value from string by state type */
int uhab_item_state_set_value(uhab_item_state_t *state, const char *buf);


/** Set item state */
int uhab_item_state_set(uhab_item_state_t *dest, const uhab_item_state_t *src);

/** Set state command */
int uhab_item_state_set_command(uhab_item_state_t *state, uhab_item_state_cmd_t cmd);

/** Get state command */
int uhab_item_state_get_command(const uhab_item_state_t *state, uhab_item_state_cmd_t *cmd);

/** Set state number */
int uhab_item_state_set_number(uhab_item_state_t *state, double number);

/** Get number */
int uhab_item_state_get_number(const uhab_item_state_t *state, double *number);

/** Set state string */
int uhab_item_state_set_string(uhab_item_state_t *state, const char *str);

/** Get state string */
int uhab_item_state_get_string(const uhab_item_state_t *state, char *buf, int bufsize);



/** Copy item state */
int uhab_item_state_copy(uhab_item_state_t *dest, const uhab_item_state_t *src);

/** Release state allocated resource */
int uhab_item_state_release(uhab_item_state_t *state);

/** Compare item state */
int uhab_item_state_compare(const uhab_item_state_t *curstate, const uhab_item_state_t *newstate);


#endif // __ITEM_STATE_H