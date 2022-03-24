
#ifndef __MINING_BINDING_H
#define __MINING_BINDING_H


#ifndef CFG_MINING_THREAD_STACK_SIZE
#define CFG_MINING_THREAD_STACK_SIZE       2048
#endif

#ifndef CFG_MINING_THREAD_PRIORITY
#define CFG_MINING_THREAD_PRIORITY         osPriorityNormal
#endif

#ifndef CFG_MINING_MAXNUM_ARGS
#define CFG_MINING_MAXNUM_ARGS          8
#endif

#ifndef CFG_MINING_QUEUE_SIZE
#define CFG_MINING_QUEUE_SIZE           16
#endif

#ifndef CFG_MINING_POLL_TIME
#define CFG_MINING_POLL_TIME            1000
#endif


#define MINING_ALL_GPU_ADDR             -1


/** Mining device item type */
typedef enum
{
   MINIG_ITEM_TYPE_NONE,
   MINIG_ITEM_TYPE_CONTROL_GPU,        // Control GPU (ON|OFF)
   MINIG_ITEM_TYPE_STATUS_INFO,        // Status info in text format
   MINIG_ITEM_TYPE_STATE,              // State ON|OFF
   MINIG_ITEM_TYPE_HASHRATE,
   MINIG_ITEM_TYPE_SHARES,

} mining_device_item_type_t;


struct mining_device;

/** Mining device items */
typedef struct mining_device_item
{
   struct mining_device_item *next;
   struct mining_device *dev;
   const uhab_item_t *item;
   int gpuaddr;
   mining_device_item_type_t type;

} mining_device_item_t;


/** Mining device */
typedef struct mining_device
{
   struct mining_device *next;
   char *name;
   char *hostname;

   /** Device items */
   LIST_STRUCT(items);

} mining_device_t;


/** Mining command */
typedef struct
{
   mining_device_item_t *devitem;
   uhab_item_state_t state;

} mining_event_t;


#endif // __MINING_BINDING_H
