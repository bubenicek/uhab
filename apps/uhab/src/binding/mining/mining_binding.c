/**
 * \file mining_binding.c        \brief Mining protocol binding
 */

#include "uhab.h"
#include "mining_binding.h"
#include "mining_jsonrpc.h"

TRACE_TAG(binding_mining);
#if !ENABLE_TRACE_MINING
#include "trace_undef.h"
#endif

// Prototypes:
static mining_device_t *alloc_mining_device(const char *name);
static int free_mining_device(mining_device_t *dev);
static void mining_thread(void *arg);


// Locals:
static const osThreadDef(MINING, mining_thread, CFG_MINING_THREAD_PRIORITY, 0, CFG_MINING_THREAD_STACK_SIZE);
static osThreadId thread;

static osPoolDef(MINING, CFG_MINING_QUEUE_SIZE, mining_event_t);
static osPoolId pool;

const osMessageQDef(MINING, CFG_MINING_QUEUE_SIZE, uint32_t);
static osMessageQId queue;

LIST(mining_devices);

static uint32_t poll_interval = 1000;


/** Initialize binding */
static int mining_binding_init(void)
{
    char value[255];

    list_init(mining_devices);

    if (uhab_config_service_get_value(CFG_MINING_BINDING_NAME, "poll", value, sizeof(value)) == 0)
    {
       poll_interval = atoi(value);
    }

    if ((queue = osMessageCreate(osMessageQ(MINING), osThreadGetId())) == NULL)
    {
        TRACE_ERROR("Create queue");
        throw_exception(fail_queue);
    }

    if ((pool = osPoolCreate(osPool(MINING))) == NULL)
    {
        TRACE_ERROR("Create pool");
        throw_exception(fail_pool);
    }

    TRACE("Init, poll_interval: %d ms", poll_interval);

    return 0;

fail_pool:
fail_queue:
    return -1;
}

/** Deinitialize binding */
static int mining_binding_deinit(void)
{
    TRACE("Deinit");
    return 0;
}

/** Start binding */
static int mining_binding_start(void)
{
    if ((thread = osThreadCreate(osThread(MINING), NULL)) == 0)
    {
        TRACE_ERROR("Start thread");
        throw_exception(fail);
    }

    TRACE("Start");

    return 0;

fail:
    return -1;
}

/** Configure binding */
static int mining_binding_configure(struct uhab_item *item, const char *binding_config)
{
    char *key, *value;
    char *params[CFG_MINING_MAXNUM_ARGS];
    int params_count = CFG_MINING_MAXNUM_ARGS;
    mining_device_t *dev = NULL;
    mining_device_item_t *devitem = NULL;

    // Get first key/value with binding type
    if (uhab_config_parse_params((char *)binding_config, &key, &value, params, &params_count) != 0)
    {
        TRACE_ERROR("Parse binding params: '%s'", binding_config);
        throw_exception(fail);
    }

    if ((dev = alloc_mining_device(value)) == NULL)
    {
        TRACE_ERROR("Alloc device: %s failed", value);
        throw_exception(fail);
    }

    item->binding.protocol_item = dev;

    if (params_count >= 2)
    {
        //for (ix = 0; ix < params_count; ix++)
        //   TRACE("   [%s]", params[ix]);

        if ((devitem = os_malloc(sizeof(mining_device_item_t))) == NULL)
        {
            TRACE_ERROR("Alloc device item");
            throw_exception(fail);
        }
        os_memset(devitem, 0, sizeof(mining_device_item_t));
        devitem->dev = dev;
        devitem->item = item;
        devitem->gpuaddr = atoi(params[0]);
        list_add(dev->items, devitem);

        item->binding.protocol_item = devitem;

        if (strcasecmp(params[1], "control-gpu") == 0)
        {
            devitem->type = MINIG_ITEM_TYPE_CONTROL_GPU;
        }
        else if (strcasecmp(params[1], "status") == 0)
        {
            devitem->type = MINIG_ITEM_TYPE_STATUS_INFO;
        }
        else if (strcasecmp(params[1], "hashrate") == 0)
        {
            devitem->type = MINIG_ITEM_TYPE_HASHRATE;
        }
        else if (strcasecmp(params[1], "shares") == 0)
        {
            devitem->type = MINIG_ITEM_TYPE_SHARES;
        }
        else
        {
            TRACE("Not supported mining method %s of parameter", params[1]);
            throw_exception(fail);
        }
    }
    else
    {
        TRACE_ERROR("Not expected number of mining paramateres, minimal 2 required");
        throw_exception(fail);
    }

    TRACE("Configure item: %s  config: %s", item->name, binding_config);

    return 0;

fail:
    if (dev != NULL)
        free_mining_device(dev);

    return -1;
}

/** Send command (item state) to binded devices */
static int mining_binding_send(const uhab_item_t *item, const uhab_item_state_t *state)
{
    mining_event_t *event = NULL;

    // Alloc event
    if ((event = osPoolAlloc(pool)) == NULL)
    {
        TRACE_ERROR("Alloc event");
        throw_exception(fail);
    }

    // Set queued item state
    event->devitem = item->binding.protocol_item;
    uhab_item_state_set(&event->state, state);

    // Add command to queue
    if (osMessagePut(queue, (uintptr_t)event, osWaitForever) != osOK)
    {
        TRACE_ERROR("Add event to queue");
        throw_exception(fail);
    }

    return 0;

fail:
    if (event != NULL)
        osPoolFree(pool, event);

    return -1;
}


static mining_device_t *alloc_mining_device(const char *name)
{
    char key[255];
    char value[255];
    mining_device_t *dev = NULL;

    // Try to find exists device
    for (dev = list_head(mining_devices); dev != NULL; dev = list_item_next(dev))
    {
        if (!strcmp(dev->name, name))
            break;
    }

    if (dev == NULL)
    {
        if ((dev = os_malloc(sizeof(mining_device_t))) == NULL)
        {
            TRACE_ERROR("Alloc device name");
            throw_exception(fail_alloc_dev);
        }
        os_memset(dev, 0, sizeof(mining_device_t));
        dev->name = os_strdup(name);
        LIST_STRUCT_INIT(dev, items);

        // Get hostname
        snprintf(key, sizeof(key), "%s.hostname", name);
        if (uhab_config_service_get_value(CFG_MINING_BINDING_NAME, key, value, sizeof(value)) != 0)
        {
            TRACE_ERROR("Not found key '%s'", key);
            throw_exception(fail_key);
        }
        dev->hostname = os_strdup(value);

        list_add(mining_devices, dev);
        TRACE("Alloc device name: %s   hostname: %s", dev->name, dev->hostname);
    }

    return dev;

fail_key:
    free_mining_device(dev);
fail_alloc_dev:
    return NULL;
}

static int free_mining_device(mining_device_t *dev)
{
    // TOOD: free all items
    os_free(dev);
    return 0;
}


static void mining_thread(void *arg)
{
    osEvent evt;
    mining_event_t *event;
    mining_device_t *dev;
    mining_device_item_t *devitem;
    mining_jsonrpc_rig_status_t miner_status;
    uint8_t online;

    TRACE("Working thread is running ...");

    while(1)
    {
        do
        {
            evt = osMessageGet(queue, poll_interval);
            if (evt.status == osEventMessage)
            {
                event = evt.value.p;

                switch(event->devitem->type)
                {
                case MINIG_ITEM_TYPE_CONTROL_GPU:
                {
                    uhab_item_state_cmd_t cmd;

                    if (uhab_item_state_get_command(&event->state, &cmd) != 0)
                    {
                        TRACE_ERROR("Get command state failed");
                        break;
                    }

                    if (mining_jsonrpc_control_gpu(event->devitem->dev->hostname, event->devitem->gpuaddr, cmd) != 0)
                    {
                        TRACE_ERROR("Control host: %s  GPU: %d to state: %d failed", event->devitem->dev->hostname, event->devitem->gpuaddr, cmd);
                        break;
                    }
                }
                break;

                default:
                    TRACE_ERROR("Not supported control item type");
                }

                // Free event
                uhab_item_state_release(&event->state);
                osPoolFree(pool, event);
            }
            else if (evt.status != osEventTimeout)
            {
                TRACE_ERROR("Get command from the queue");
            }

        } while(evt.status == osEventMessage);


        // Poll all devices
        for (dev = list_head(mining_devices); dev != NULL; dev = list_item_next(dev))
        {
            if (mining_jsonrpc_get_miner_status(dev->hostname, &miner_status) == 0)
            {
               online = 1;
            }
            else
            {
                TRACE("Miner: %s is offline", dev->hostname);
                online = 0;
            }

            // Update states of all items
            for (devitem = list_head(dev->items); devitem != NULL; devitem = list_item_next(devitem))
            {
                switch(devitem->type)
                {
                case MINIG_ITEM_TYPE_CONTROL_GPU:
                {
                    uhab_item_state_t newstate = UHAB_ITEM_STATE_INIT(UHAB_ITEM_STATE_TYPE_CMD);

                    if (devitem->gpuaddr == MINING_ALL_GPU_ADDR)
                    {
                        uhab_item_state_set_command(&newstate, (online && miner_status.total_hashrate) > 0 ? 1 : 0);
                    }
                    else
                    {
                        ASSERT(devitem->gpuaddr < CFG_MINIG_MAXNUM_GPU);
                        uhab_item_state_set_command(&newstate, (online && miner_status.gpu[devitem->gpuaddr].hashrate > 0) ? 1 : 0);
                    }

                    // Update item only when it was changed
                    if (uhab_item_state_compare(&devitem->item->state, &newstate) != 0)
                    {
                        if (uhab_bus_update(devitem->item, &newstate) != 0)
                        {
                            TRACE_ERROR("Update item failed");
                        }
                    }
                }
                break;

                case MINIG_ITEM_TYPE_STATUS_INFO:
                {
                    char value[64];
                    uhab_item_state_t newstate = UAHB_ITEM_STATE_INIT_STRING_REF(value);

                    if (online)
                    {
                       if (devitem->gpuaddr == MINING_ALL_GPU_ADDR)
                       {
                           int min, hour, days;

                           min = miner_status.uptime % 60;
                           miner_status.uptime /= 60;
                           hour = miner_status.uptime % 24;
                           miner_status.uptime /= 24;
                           days = miner_status.uptime % 24;

                           if (days > 0)
                               snprintf(value, sizeof(value), "%d Mh/s  %d  %dd %02d:%02d", miner_status.total_hashrate / 1000, miner_status.total_shares, days, hour, min);
                           else
                               snprintf(value, sizeof(value), "%d Mh/s  %d  %02d:%02d", miner_status.total_hashrate / 1000, miner_status.total_shares, hour, min);
                       }
                       else
                       {
                           ASSERT(devitem->gpuaddr < CFG_MINIG_MAXNUM_GPU);

                           snprintf(value, sizeof(value), "%d Mh/s  %d °C  %d %%%%",
                                    miner_status.gpu[devitem->gpuaddr].hashrate / 1000,
                                    miner_status.gpu[devitem->gpuaddr].temperature,
                                    miner_status.gpu[devitem->gpuaddr].fanspeed);
                       }
                     }
                     else
                     {
                        snprintf(value, sizeof(value), "OFFLINE");
                     }

                    // Update item only when it was changed
                    if (uhab_item_state_compare(&devitem->item->state, &newstate) != 0)
                    {
                        if (uhab_bus_update(devitem->item, &newstate) != 0)
                        {
                            TRACE_ERROR("Update item failed");
                        }
                    }
                }
                break;

                default:
                    // TODO:
                    break;
                }
            }
        }
    }
}

/** Interface definition */
uhab_protocol_binding_t mining_binding =
{
    .name = CFG_MINING_BINDING_NAME,
    .label = "Mining client",
    .init = mining_binding_init,
    .deinit = mining_binding_deinit,
    .start = mining_binding_start,
    .configure = mining_binding_configure,
    .send_command = mining_binding_send
};
