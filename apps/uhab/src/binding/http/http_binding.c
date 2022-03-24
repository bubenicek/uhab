/**
 * \file http_binding.c        \brief HTTP protocol binding
 */

#include "uhab.h"
#include "httpd_socket.h"

TRACE_TAG(binding_http);
#if !ENABLE_TRACE_HTTP
#include "trace_undef.h"
#endif

#define CFG_HTTP_MAXNUM_ARGS     8


typedef struct
{
   const uhab_item_t *item;
   uhab_item_state_t state;

} http_event_t;


typedef enum
{
   HTTP_DEVICE_METHOD_GET,
   HTTP_DEVICE_METHOD_POST,
   HTTP_DEVICE_METHOD_PUT

} http_device_method_t;


/** HTTP device */
typedef struct http_device
{
   struct http_device *next;

   char *name;
   char *hostname;
   int port;
   http_device_method_t method;
   char *url;

} http_device_t;


// Prototypes:
static http_device_t *alloc_http_device(const char *name);
static int free_http_device(http_device_t *dev);
static void http_thread(void *arg);


// Locals:
static const osThreadDef(HTTP, http_thread, CFG_HTTP_THREAD_PRIORITY, 0, CFG_HTTP_THREAD_STACK_SIZE);
static osThreadId thread;

static osPoolDef(HTTP, CFG_UHAB_HTTP_QUEUE_SIZE, http_event_t);
static osPoolId pool;

const osMessageQDef(HTTP, CFG_UHAB_HTTP_QUEUE_SIZE, uint32_t);
static osMessageQId queue;

LIST(http_devices);


/** Initialize binding */
static int http_binding_init(void)
{
   list_init(http_devices);

   if ((queue = osMessageCreate(osMessageQ(HTTP), osThreadGetId())) == NULL)
   {
      TRACE_ERROR("Create queue");
      throw_exception(fail_queue);
   }

   if ((pool = osPoolCreate(osPool(HTTP))) == NULL)
   {
      TRACE_ERROR("Create pool");
      throw_exception(fail_pool);
   }

   if ((thread = osThreadCreate(osThread(HTTP), NULL)) == 0)
   {
      TRACE_ERROR("Start thread");
      throw_exception(fail_thread);
   }

   TRACE("Init");

   return 0;

fail_thread:
fail_pool:
fail_queue:
   return -1;
}

/** Deinitialize binding */
static int http_binding_deinit(void)
{
   TRACE("Deinit");
   return 0;
}

/** Start binding */
static int http_binding_start(void)
{
   TRACE("Start");
   return 0;
}

/** Configure binding */
static int http_binding_configure(struct uhab_item *item, const char *binding_config)
{
   char *key, *value;
   char *params[CFG_HTTP_MAXNUM_ARGS];
   int params_count = CFG_HTTP_MAXNUM_ARGS;
   http_device_t *dev = NULL;

   // Get first key/value with binding type
   if (uhab_config_parse_params((char *)binding_config, &key, &value, params, &params_count) != 0)
   {
      TRACE_ERROR("Parse binding params: '%s'", binding_config);
      throw_exception(fail);
   }

   if ((dev = alloc_http_device(value)) == NULL)
   {
      TRACE_ERROR("Alloc http device: %s failed", value);
      throw_exception(fail);
   }

   item->binding.protocol_item = dev;

   if (params_count > 0)
   {
      if ((dev->url = os_strdup(params[0])) == NULL)
      {
         TRACE_ERROR("Alloc url string");
         throw_exception(fail);
      }
   }

   TRACE("Configure item: %s  config: %s", item->name, binding_config);

   return 0;

fail:
   if (dev != NULL)
      free_http_device(dev);
   return -1;
}

/** Send command (item state) to binded devices */
static int http_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
   http_event_t *event;

   // Alloc event
   if ((event = osPoolAlloc(pool)) == NULL)
   {
      TRACE_ERROR("Alloc event");
      return -1;
   }

   // Set queued item state
   event->item = item;
   uhab_item_state_set(&event->state, state);

   // Add command to queue
   if (osMessagePut(queue, (uintptr_t)event, osWaitForever) != osOK)
   {
      TRACE_ERROR("Add event to queue");
      osPoolFree(pool, event);
      return -1;
   }

   return 0;
}


static http_device_t *alloc_http_device(const char *name)
{
   char key[255];
   char value[255];
   http_device_t *dev = NULL;

   if ((dev = os_malloc(sizeof(http_device_t))) == NULL)
   {
      TRACE_ERROR("Alloc device name");
      throw_exception(fail_alloc_dev);
   }
   os_memset(dev, 0, sizeof(http_device_t));
   dev->name = os_strdup(name);

   // Get hostname
   snprintf(key, sizeof(key), "%s.hostname", name);
   if (uhab_config_service_get_value(CFG_HTTP_BINDING_NAME, key, value, sizeof(value)) != 0)
   {
      TRACE_ERROR("Not found key '%s'", key);
      throw_exception(fail_key);
   }
   dev->hostname = os_strdup(value);

   // TODO: method reading
   dev->method = HTTP_DEVICE_METHOD_GET;

   // Get port
   snprintf(key, sizeof(key), "%s.port", name);
   if (uhab_config_service_get_value(CFG_HTTP_BINDING_NAME, key, value, sizeof(value)) == 0)
   {
      dev->port = atoi(value);
   }
   else
   {
      dev->port = 80;
   }
   
   list_add(http_devices, dev);
   TRACE("Alloc HTTP device name: %s   hostname: %s", dev->name, dev->hostname);

   return dev;

fail_key:
   free_http_device(dev);
fail_alloc_dev:
   return NULL;
}

static int free_http_device(http_device_t *dev)
{
   free(dev);
   return 0;
}


static void http_thread(void *arg)
{
   osEvent evt;
   http_event_t *event;
   int sd = -1;
   int len;
   http_device_t *dev;
   char buf[255];

   TRACE("HTTP thread is running ...");

   while(1)
   {
      // Wait for event in the queue
      evt = osMessageGet(queue, osWaitForever);
      if (evt.status != osEventMessage)
      {
         TRACE_ERROR("Get event from the queue");
         continue;
      }

      event = evt.value.p;
      dev = event->item->binding.protocol_item;

      if ((sd = httpd_raw_socket_connect(dev->hostname, dev->port)) < 0)
      {
         TRACE_ERROR("Connect to %s", dev->hostname);
         throw_exception(fail);
      }

      len = snprintf(buf, sizeof(buf), "GET %s", dev->url);
      if (httpd_raw_socket_send(sd, buf, len) < 0)
      {
         TRACE_ERROR("Send request");
         throw_exception(fail);
      }

      uhab_item_state_get_value(&event->state, buf, sizeof(buf));

      if (httpd_raw_socket_send(sd, buf, strlen(buf)) < 0)
      {
         TRACE_ERROR("Send data");
         throw_exception(fail);
      }

      if (httpd_raw_socket_send(sd, " HTTP/1.0\r\n\r\n", 13) < 0)
      {
         TRACE_ERROR("Send data end");
         throw_exception(fail);
      }

      TRACE("Send to item: %s  data: %s%s -> %s", event->item->name, dev->url, uhab_item_state_get_value(&event->state, buf, sizeof(buf)), dev->hostname);

fail:
      if (sd != -1)
         httpd_raw_socket_close(sd);

      // Free event
      uhab_item_state_release(&event->state);
      osPoolFree(pool, event);
   }
}


/** Interface definition */
uhab_protocol_binding_t http_binding =
{
   .name = CFG_HTTP_BINDING_NAME,
   .label = "HTTP client",
   .init = http_binding_init,
   .deinit = http_binding_deinit,
   .start = http_binding_start,
   .configure = http_binding_configure,
   .send_command = http_binding_send
};
