
#ifndef __MODBUS_BINDING_H
#define __MODBUS_BINDING_H

#ifndef CFG_MODBUS_BINDING_NAME
#define CFG_MODBUS_BINDING_NAME              "modbus"
#endif

#ifndef CFG_MODBUS_BINDING_MAX_ITEMS
#define CFG_MODBUS_BINDING_MAX_ITEMS         16
#endif

#ifndef CFG_MODBUS_BINDING_MAX_DEVICES
#define CFG_MODBUS_BINDING_MAX_DEVICES       16
#endif

#ifndef CFG_MODBUS_DEFAULT_POLL_INTERVAL
#define CFG_MODBUS_DEFAULT_POLL_INTERVAL     250
#endif

#ifndef CFG_MODBUS_BINDING_CFG_MAX_ARGS
#define CFG_MODBUS_BINDING_CFG_MAX_ARGS      16
#endif

#ifndef CFG_MODBUS_BINDING_CMD_QUEUE_SIZE
#define CFG_MODBUS_BINDING_CMD_QUEUE_SIZE    32
#endif


/** Modbus device bus type */
typedef enum
{
   MODBUS_DEVICE_BUS_SERIAL,
   MODBUS_DEVICE_BUS_TCP
      
} modbus_device_bus_type_t;


/** MOdbus device function type */
typedef enum
{
   MODBUS_DEVICE_FUNC_COIL,
   MODBUS_DEVICE_FUNC_DISCRETE,
   MODBUS_DEVICE_FUNC_HOLDING
   
} modbus_device_func_type_t;


/** Serial device definition */
typedef struct
{
   hal_uart_t uart;
   uint32_t baudrate;
   uint16_t inter_transaction_delay;
   uint16_t receive_timeout;
   
} modbus_serial_device_t;


/** TCP device definition */
typedef struct
{
   
} modbus_tcp_device_t;


/** Device item definition */
typedef struct
{
   struct modbus_device *dev;
   const uhab_item_t *item;
   uint16_t index;
   uhab_item_state_t state;
   
} modbus_device_item_t;


/** Modbus device definition */
typedef struct modbus_device
{
   char *name;
   modbus_device_bus_type_t bus_type;
   modbus_device_func_type_t func_type;
   uint16_t id;
   uint16_t start;
   uint16_t length;
   uint16_t poll_interval;

   uint32_t poll_tmo;
   
   modbus_device_item_t items[CFG_MODBUS_BINDING_MAX_ITEMS];
   int items_count;
   
   union
   {
      modbus_serial_device_t serial;
      modbus_tcp_device_t tcp;   
   };
   
} modbus_device_t;


/** Modbus command types */
typedef enum
{
   MODBUS_CMD_SET_COIL,
   MODBUS_CMD_WRITE_HOLDING,
   
} modbus_cmd_type_t;


/** Modbus command */
typedef struct
{
   modbus_cmd_type_t type;
   
   union
   {
      struct
      {
         modbus_device_t *dev;
         modbus_device_item_t *devitem;
         uint16_t coil;       // Coil index 0 - 16
         uint8_t state;       // State 0/1
    
      } set_coil;
      
      struct
      {
         modbus_device_t *dev;
         modbus_device_item_t *devitem;
         uint16_t regaddr;
         uint16_t regval;
         
      } write_holding;
   };
   
} modbus_cmd_t;


#endif // __MODBUS_BINDING_H