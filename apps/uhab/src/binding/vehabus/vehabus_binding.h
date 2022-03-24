
#ifndef __VEHABUS_BINDING_H
#define __VEHABUS_BINDING_H


#ifndef CFG_VEHABUS_INPUT_POLL_TIME
#define CFG_VEHABUS_INPUT_POLL_TIME       20
#endif // CFG_VEHABUS_INPUT_POLL_TIME

#ifndef CFG_VEHABUS_ONEWIRE_POLL_TIME
#define CFG_VEHABUS_ONEWIRE_POLL_TIME     1000
#endif

#ifndef CFG_VEHABUS_MAXNUM_ARGS
#define CFG_VEHABUS_MAXNUM_ARGS           4
#endif


typedef enum
{
   VEHABUS_ITEM_TYPE_INPUT,
   VEHABUS_ITEM_TYPE_OUTPUT,
   VEHABUS_ITEM_TYPE_OCINPUT,
   VEHABUS_ITEM_TYPE_OCOUTPUT,
   VEHABUS_ITEM_TYPE_ADC,
   VEHABUS_ITEM_TYPE_RELAY,
   VEHABUS_ITEM_TYPE_ONEWIRE,
   
} vehabus_item_type_t;


typedef struct vehabus_item
{
   struct vehabus_item *next;

   const uhab_item_t *item;
   vehabus_item_type_t type;
   int index;
   
} vehabus_item_t;



#endif // __VEHABUS_BINDING_H