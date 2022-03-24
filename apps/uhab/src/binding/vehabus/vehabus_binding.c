/**
 * \file vehabus_binding.c        \brief VEHABUS bindings
 */

#include "uhab.h"
#include "vehabus_binding.h"

#if defined (CFG_VEHABUS_ENABLED) && (CFG_VEHABUS_ENABLED == 1)

#define CFG_VEHABUS_DEGLITCH_COUNT           3
#define CFG_VEHABUS_MAX_DEGLITCH_COUNT       255

TRACE_TAG(binding_vehabus);
#if !ENABLE_TRACE_VEHABUS
#include "trace_undef.h"
#endif

// Prototypes:
static void input_timer_cb(void *arg);
static void onewire_timer_cb(void *arg);

// Locals:
static osTimerId input_timer;
static int input_timer_interval = CFG_VEHABUS_INPUT_POLL_TIME;
static uint32_t vehabus_input_mask = 0;
static uint64_t vehabus_output_mask = 0;

static int input_deglitch_count = CFG_VEHABUS_DEGLITCH_COUNT;

static osTimerId onewire_timer;
static int onewire_timer_interval = CFG_VEHABUS_ONEWIRE_POLL_TIME;

LIST(input_items);
LIST(output_items);
LIST(relay_items);
LIST(onewire_items);
LIST(ocinput_items);
LIST(ocoutput_items);
LIST(adc_items);


/** Initialize binding */
static int vehabus_binding_init(void)
{
   char value[255];

   list_init(input_items);
   list_init(output_items);
   list_init(relay_items);
   list_init(onewire_items);
   list_init(ocinput_items);
   list_init(ocoutput_items);
   list_init(adc_items);

   // Get configuration
   if (uhab_config_service_get_value(CFG_VEHABUS_BINDING_NAME, "input.poll", value, sizeof(value)) == 0)
      input_timer_interval = atoi(value);

   if (uhab_config_service_get_value(CFG_VEHABUS_BINDING_NAME, "input.deglitch_filter", value, sizeof(value)) == 0)
   {
      input_deglitch_count = atoi(value);
      if (input_deglitch_count > CFG_VEHABUS_MAX_DEGLITCH_COUNT)
      {
         TRACE_ERROR("Deglitch filter exceeded max value %d", CFG_VEHABUS_MAX_DEGLITCH_COUNT);
         input_deglitch_count = CFG_VEHABUS_MAX_DEGLITCH_COUNT;
      }
   }

   if (uhab_config_service_get_value(CFG_VEHABUS_BINDING_NAME, "onewire.poll", value, sizeof(value)) == 0)
      onewire_timer_interval = atoi(value);


   TRACE("Init");

   return 0;
}

/** Deinitialize binding */
static int vehabus_binding_deinit(void)
{
   TRACE("Deinit");
   return 0;
}

/** Start binding */
static int vehabus_binding_start(void)
{
   const osTimerDef(INPUT_TIMER, input_timer_cb);
   const osTimerDef(ONEWIRE_TIMER, onewire_timer_cb);

   // Start inputs
   if (list_length(input_items) > 0)
   {
      if ((input_timer = osTimerCreate(osTimer(INPUT_TIMER), osTimerPeriodic, NULL)) == NULL)
      {
         TRACE_ERROR("Create input timer failed");
         return -1;
      }

      if (osTimerStart(input_timer, input_timer_interval) != osOK)
      {
         TRACE_ERROR("Start input timer failed");
         return -1;
      }

      TRACE("Start input polling time: %d  deglitch_filter: %d", input_timer_interval, input_deglitch_count);
   }

   // Start onewire
   if (list_length(onewire_items) > 0)
   {
      if ((onewire_timer = osTimerCreate(osTimer(ONEWIRE_TIMER), osTimerPeriodic, NULL)) == NULL)
      {
         TRACE_ERROR("Create onewire timer failed");
         return -1;
      }

      if (osTimerStart(onewire_timer, onewire_timer_interval) != osOK)
      {
         TRACE_ERROR("Start onewire timer failed");
         return -1;
      }

      // Initialize temperature items
      onewire_timer_cb(NULL);

      TRACE("Start onewire polling time: %d", onewire_timer_interval);
   }

   return 0;
}

/** Configure binding */
static int vehabus_binding_configure(struct uhab_item *item, const char *binding_config)
{
   char *key, *value;
   char *params[CFG_VEHABUS_MAXNUM_ARGS];
   int params_count = CFG_VEHABUS_MAXNUM_ARGS;
   vehabus_item_t *vi = NULL;

   // Get first key/value with binding type
   if (uhab_config_parse_params((char *)binding_config, &key, &value, params, &params_count) != 0)
   {
      TRACE_ERROR("Parse binding params: '%s'", binding_config);
      throw_exception(fail);
   }

   if (params_count == 0)
   {
      TRACE_ERROR("Bad number vehabus params of item: %s", item->name);
      throw_exception(fail);
   }

   if ((vi = os_malloc(sizeof(vehabus_item_t))) == NULL)
   {
      TRACE_ERROR("Alloc vehabus item");
      throw_exception(fail);
   }
   os_memset(vi, 0, sizeof(vehabus_item_t));
   vi->item = item;
   item->binding.protocol_item = vi;

   if (!strcasecmp(value, "input"))
      vi->type = VEHABUS_ITEM_TYPE_INPUT;
   else if (!strcasecmp(value, "output"))
      vi->type = VEHABUS_ITEM_TYPE_OUTPUT;
   else if (!strcasecmp(value, "onewire"))
      vi->type = VEHABUS_ITEM_TYPE_ONEWIRE;
   else if (!strcasecmp(value, "relay"))
      vi->type = VEHABUS_ITEM_TYPE_RELAY;
   else if (!strcasecmp(value, "ocoutput"))
      vi->type = VEHABUS_ITEM_TYPE_OCOUTPUT;
   else if (!strcasecmp(value, "ocinput"))
      vi->type = VEHABUS_ITEM_TYPE_OCINPUT;
   else if (!strcasecmp(value, "adc"))
      vi->type = VEHABUS_ITEM_TYPE_ADC;
   else
   {
      TRACE_ERROR("Not supported vehabus type: %s of item: %s", value, item->name);
      throw_exception(fail);
   }

   vi->index = atoi(params[0]);

   // Add to list by type
   switch(vi->type)
   {
      case VEHABUS_ITEM_TYPE_INPUT:
         if (vi->index >= VEHABUS_GPIO_NUM)
         {
            TRACE_ERROR("Vehabus input index: %d of item: %s exceeded max number %d", vi->index, item->name, VEHABUS_GPIO_NUM);
            throw_exception(fail);
         }
         list_add(input_items, vi);
         break;

      case VEHABUS_ITEM_TYPE_OUTPUT:
         if (vi->index >= VEHABUS_LED_GPIO_NUM)
         {
            TRACE_ERROR("Vehabus output index: %d of item: %s exceeded max number %d", vi->index, item->name, VEHABUS_LED_GPIO_NUM);
            throw_exception(fail);
         }
         list_add(output_items, vi);
         break;

      case VEHABUS_ITEM_TYPE_OCINPUT:
         if (vi->index >= VEHABUS_OC_INPUT_GPIO_NUM)
         {
            TRACE_ERROR("Vehabus ocinput index: %d of item: %s exceeded max number %d", vi->index, item->name, VEHABUS_OC_INPUT_GPIO_NUM);
            throw_exception(fail);
         }
         list_add(ocinput_items, vi);
         break;

      case VEHABUS_ITEM_TYPE_OCOUTPUT:
         if (vi->index >= VEHABUS_OC_OUTPUT_GPIO_NUM)
         {
            TRACE_ERROR("Vehabus ocoutput index: %d of item: %s exceeded max number %d", vi->index, item->name, VEHABUS_OC_OUTPUT_GPIO_NUM);
            throw_exception(fail);
         }
         list_add(ocoutput_items, vi);
         break;

      case VEHABUS_ITEM_TYPE_ADC:
         if (vi->index >= VEHABUS_ADC_NUM)
         {
            TRACE_ERROR("Vehabus adc index: %d of item: %s exceeded max number %d", vi->index, item->name, VEHABUS_ADC_NUM);
            throw_exception(fail);
         }
         list_add(adc_items, vi);
         break;

      case VEHABUS_ITEM_TYPE_ONEWIRE:
         list_add(onewire_items, vi);
         break;

      case VEHABUS_ITEM_TYPE_RELAY:
         if (vi->index >= VEHABUS_GPIO_RELAYS_NUM)
         {
            TRACE_ERROR("Vehabus relay index: %d of item: %s exceeded max number %d", vi->index, item->name, VEHABUS_GPIO_RELAYS_NUM);
            throw_exception(fail);
         }
         list_add(relay_items, vi);
         break;
   }

   TRACE("Configure item: %s  vehabus_type: %d  vehabus_index: %d  config: %s", item->name, vi->type, vi->index, binding_config);

   return 0;

fail:
   if (vi != NULL)
      os_free(vi);
   return -1;
}

/** Send command (item state) to binded devices */
static int vehabus_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   vehabus_item_t *vi;

   vi = item->binding.protocol_item;
   ASSERT(vi != NULL);

   if (state->type != UHAB_ITEM_STATE_TYPE_CMD)
   {
      TRACE_ERROR("Not supported state type: %d of item: %s, only command", state->type, item->name);
      return -1;
   }

   switch(vi->type)
   {
      case VEHABUS_ITEM_TYPE_OUTPUT:
      {
         if (state->value.cmd == UHAB_ITEM_STATE_CMD_ON)
            vehabus_output_mask |= (1 << vi->index);
         else
            vehabus_output_mask &= ~(1 << vi->index);
      }
      break;

      case VEHABUS_ITEM_TYPE_OCOUTPUT:
         hal_gpio_set(VEHABUS_OC_OUTPUT_GPIO_START + vi->index, (state->value.cmd == UHAB_ITEM_STATE_CMD_ON));
         break;

      case VEHABUS_ITEM_TYPE_RELAY:
         hal_gpio_set(VEHABUS_GPIO_RELAY_START + vi->index, (state->value.cmd == UHAB_ITEM_STATE_CMD_ON));
         break;

      default:
         TRACE_ERROR("Not supported send command to item: %s  vehabus type: %d", item->name, vi->type);
         return -1;
   }

   // Update item state
   return uhab_bus_update(item, state);
}

/** Reading inputs state timer callback */
static void input_timer_cb(void *arg)
{
   int ix, iy, di, di_high, di_low;
   hal_gpio_t gpio;
   vehabus_item_t *vi;
   uhab_item_state_t newstate;
   int pwr1_on = 0;
   int pwr2_on = 0;
   uint32_t prev_input_mask = vehabus_input_mask;
   uint32_t input_mask[CFG_VEHABUS_MAX_DEGLITCH_COUNT];

   // Read inputs
   for (di = 0; di < input_deglitch_count; di++)
   {
      // Reset port state (set as output in LOW state)
      for (gpio = VEHABUS_GPIO_START, ix = 0; ix < VEHABUS_GPIO_NUM; ix++, gpio++)
         hal_gpio_configure(gpio, HAL_GPIO_MODE_OUTPUT);

      input_mask[di] = 0;

      // Set as input and read input state
      for (ix = 0, gpio = VEHABUS_GPIO_START; ix < VEHABUS_GPIO_NUM; ix++, gpio++)
      {
         hal_gpio_configure(gpio, HAL_GPIO_MODE_INPUT);
         if (hal_gpio_get(gpio))
            input_mask[di] |= (1 << ix);
      }
   }

   // Check deglitched inputs
   for (ix = 0; ix < VEHABUS_GPIO_NUM; ix++)
   {
      di_high = di_low = 0;
      for (di = 0; di < input_deglitch_count; di++)
      {
         if (input_mask[di] & (1 << ix))
            di_high++;
         else
            di_low++;
      }

      if (di_high == input_deglitch_count)
         vehabus_input_mask |= (1 << ix);
      else if (di_low == input_deglitch_count)
         vehabus_input_mask &= ~(1 << ix);
   }

   // Set current output state
   for (gpio = VEHABUS_GPIO_START, ix = 0; ix < VEHABUS_LED_GPIO_NUM; gpio += 4)
   {
      // PWR1 LEDs
      for (iy = 0; iy < 4; iy++, ix++)
      {
         hal_gpio_configure(gpio + iy, HAL_GPIO_MODE_OUTPUT);
         hal_gpio_set(gpio + iy, (vehabus_output_mask & (1 << ix)) ? 1 : 0);
         if (vehabus_output_mask & (1 << ix))
            pwr1_on++;
      }

      // PWR2 LEDs
      for (iy = 0; iy < 4; iy++, ix++)
      {
         if (vehabus_output_mask & (1 << ix))
         {
            hal_gpio_configure(gpio + iy, HAL_GPIO_MODE_OUTPUT);
            hal_gpio_set(gpio + iy, 1);
            pwr2_on++;
         }
      }
   }

   // Enable LED power
   hal_gpio_set(VEHABUS_GPIO_PWR1, (pwr1_on > 0) ? 1 : 0);
   hal_gpio_set(VEHABUS_GPIO_PWR2, (pwr2_on > 0) ? 1 : 0);

   // Test changes
   if (vehabus_input_mask != prev_input_mask)
   {
      for (vi = list_head(input_items); vi != NULL; vi = list_item_next(vi))
      {
         if ((vehabus_input_mask & (1 << vi->index)) != (prev_input_mask & (1 << vi->index)))
         {
            uhab_item_state_set_command(&newstate, ((vehabus_input_mask & (1 << vi->index)) != 0));
            VERIFY(uhab_bus_update(vi->item, &newstate) == 0);
         }
      }
   }
}

/** Reading temp from onewire sensors */
static void onewire_timer_cb(void *arg)
{
   vehabus_item_t *vi;
   uhab_item_state_t newstate;
   double temp;

   for (vi = list_head(onewire_items); vi != NULL; vi = list_item_next(vi))
   {
      temp = 21 + ((rand() % 10) * 0.1);
      uhab_item_state_set_number(&newstate, temp);
      VERIFY(uhab_bus_update(vi->item, &newstate) == 0);
   }
}

#else

static int vehabus_binding_init(void)
{
   return 0;
}

static int vehabus_binding_deinit(void)
{
   return 0;
}

static int vehabus_binding_start(void)
{
   return 0;
}

static int vehabus_binding_configure(struct uhab_item *item, const char *binding_config)
{
   return 0;
}

static int vehabus_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   return 0;
}

#endif   // CFG_VEHABUS_ENABLED

/** VEHABUS binding interface definition */
uhab_protocol_binding_t vehabus_binding =
{
   .name = CFG_VEHABUS_BINDING_NAME,
   .label = "VEHABUS",
   .init = vehabus_binding_init,
   .deinit = vehabus_binding_deinit,
   .start = vehabus_binding_start,
   .configure = vehabus_binding_configure,
   .send_command = vehabus_binding_send
};


