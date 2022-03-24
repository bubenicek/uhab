
#ifndef __DMX_BINDING_H
#define __DMX_BINDING_H


#ifndef CFG_DMX_NUM_CHANNELS
#define CFG_DMX_NUM_CHANNELS              512
#endif

#ifndef CFG_DMX_MAX_CHANNEL_VALUE
#define CFG_DMX_MAX_CHANNEL_VALUE         255
#endif

#ifndef CFG_DMX_MIN_NUM_CHANNELS
#define CFG_DMX_MIN_NUM_CHANNELS          8
#endif

// Max. dmx channles per item
#ifndef CFG_DMX_MAX_NUM_CHANNELS
#define CFG_DMX_MAX_NUM_CHANNELS          16
#endif // CFG_DMX_NUM_CHANNELS

#ifndef CFG_DMX_POLL_INTERVAL
#define CFG_DMX_POLL_INTERVAL             20
#endif // CFG_DMX_POLL_INTERVAL

#ifndef CFG_DMX_FADETICK
#define CFG_DMX_FADETICK                  CFG_DMX_POLL_INTERVAL
#endif // CFG_DMX_POLL_INTERVAL

#ifndef CFG_DMX_MAX_NUM_COMMANDS
#define CFG_DMX_MAX_NUM_COMMANDS          16
#endif // CFG_DMX_MAX_NUM_COMMANDS

#ifndef CFG_DMX_MAX_NUM_ARGS
#define CFG_DMX_MAXNUM_ARGS               16
#endif // CFG_DMX_MAX_NUM_ARGS

#ifndef CFG_DMX_PERIODICAL_FADETIME
#define CFG_DMX_PERIODICAL_FADETIME       6000
#endif

#ifndef CFG_DMX_PERIODICAL_UPDATE_ITEM_VALUE_TIME
#define CFG_DMX_PERIODICAL_UPDATE_ITEM_VALUE_TIME    500
#endif

// 100 usec mark after break delay
#ifndef CFG_DMX_MARK_AFTER_BREAK_DELAY
#define CFG_DMX_MARK_AFTER_BREAK_DELAY    20000
#endif


/** DMX command */
typedef struct
{
   /** Item command */
   uhab_item_state_cmd_t cmdtype;  
   
   /** Channel values */
   uint8_t default_channel_values[CFG_DMX_MAX_NUM_CHANNELS];
   uint8_t channel_values[CFG_DMX_MAX_NUM_CHANNELS];

   /** Fade time in ms */
   uint32_t fadetime;    
   
} dmx_cmd_t;


/** DMX device item */
typedef struct dmx_device_item
{
   struct dmx_device_item *next;

   /** Ptr to DMX device */
   struct dmx_device *dev;
   
   /** UHAB item */
   const uhab_item_t *item;

   /** Used DMX channels */
   uint16_t channels[CFG_DMX_MAX_NUM_CHANNELS];
   int channels_count;
   uint8_t channels_values[CFG_DMX_MAX_NUM_CHANNELS];

   /** Defined item commands */
   dmx_cmd_t commands[CFG_DMX_MAX_NUM_COMMANDS];
   int commands_count;

   struct
   {
      osTimerId timer;
      osMutexId mutex;
      int periodical;
      int interval;
      int step;
      int maxcount;
      int count;
      uint32_t sectimer;      
      uint32_t start_time;
      int endvalue;
      
   } fader;
   
} dmx_device_item_t;


/** DMX device */
typedef struct dmx_device
{
   struct dmx_device *next;
   
   /** Device name */
   char *name;
  
   /** DMX physical device */
   hal_dmx_t dmx;
    
   /** DMX output buffer */
   uint8_t dmxbuf[CFG_DMX_NUM_CHANNELS];
   int dmxbuf_length;
  
   /** DMX device items */
   LIST_STRUCT(items);
   
} dmx_device_t;


typedef struct
{
   
} dmx_refresh_queue_item_t;


#endif // __DMX_BINDING_H