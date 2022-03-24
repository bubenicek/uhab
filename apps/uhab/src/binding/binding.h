/**
 * \file binding.h         \brief uHAB protocol binding
 */

#ifndef __UHAB_BINDING_H
#define __UHAB_BINDING_H


/**
 * Declare the name of a binding.
 *
 * This macro is typically used in header files to declare the name of
 * a binding that is implemented in the C file.
 *
 */
#define BINDING_NAME(name) extern uhab_protocol_binding_t name


/** Binding protocol driver */
typedef struct uhab_protocol_binding
{
   struct uhab_protocol_binding *next;

   /** Binding name */
   const char *name;

   /** Binding label */
   const char *label;

   /** Initialize binding */
   int (*init)(void);

   /** Deinitialize binding */
   int (*deinit)(void);

   /** Start binding */
   int (*start)(void);

   /** Configure item binding */
   int (*configure)(struct uhab_item *item, const char *binding_config);

   /** Send command */
   int (*send_command)(const struct uhab_item *item, const uhab_item_state_t *state);

} uhab_protocol_binding_t;


/** Initialize all configured bindings */
int uhab_binding_init(void);

/** Deinitialize bindings */
int uhab_binding_deinit(void);

/** Start configured bindings */
int uhab_binding_start(void);

/** Get protocol binding by name */
const uhab_protocol_binding_t *uhab_binding_get_by_name(const char *name);

/** Get all bindings */
list_t uhab_binding_get_all_bindings(void);


#endif // __UHAB_BINDING_H
