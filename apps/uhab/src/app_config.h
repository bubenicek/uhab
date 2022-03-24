/**
 * \file config.h       \brief uHAB server static configuration
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CFG_FW_VERSION_MAJOR        0
#define CFG_FW_VERSION_MINOR        35

//-----------------------------------------------------------------------------
//                      System configuration
//-----------------------------------------------------------------------------
#define CFG_DEBUG_MEM               0
#define CFG_DEBUG_TIMESTAMP         1
#define CFG_DEBUG_HAS_FLOAT         1
#define CFG_CMSIS_OS_API            1
#define CFG_HAL_WDG_ENABLED         0
#define CFG_TRACE_TIMESTAMP_SYSTIME 1

// Shutdown handler invoked when CTRL+C is pressed
extern void uhab_system_shutdown(void);
#define CFG_APP_SHUTDOWN_HANDLER    uhab_system_shutdown

//-----------------------------------------------------------------------------
//                      Debug trace configuration
//-----------------------------------------------------------------------------
#define CFG_ENABLE_TRACE            1

#define ENABLE_TRACE_HAL            1
#define ENABLE_TRACE_FSLOG          1

#define ENABLE_TRACE_MAIN           1
#define ENABLE_TRACE_CONFIG         1
#define ENABLE_TRACE_BUS            1
#define ENABLE_TRACE_BUS_CHANGES    1
#define ENABLE_TRACE_UIPROVIDER     1
#define ENABLE_TRACE_REST_API       1

#define ENABLE_TRACE_BINDING        1
#define ENABLE_TRACE_VEHABUS        1
#define ENABLE_TRACE_MODBUS         1
#define ENABLE_TRACE_MODBUS_DATA    0
#define ENABLE_TRACE_DMX            1
#define ENABLE_TRACE_SYSTEM         1
#define ENABLE_TRACE_SNMP           1
#define ENABLE_TRACE_HTTP           0
#define ENABLE_TRACE_MINING         1
#define ENABLE_TRACE_MINING_JSONRPC 1
#define ENABLE_TRACE_EBUS           1

#define ENABLE_TRACE_AUTOMATION     1
#define ENABLE_TRACE_JSCRIPT        1
#define ENABLE_TRACE_REPOSITORY     1

#define ENABLE_TRACE_SNMP_CLIENT    0
#define ENABLE_TRACE_HTTPD          0
#define ENABLE_TRACE_HTTPD_REST     0


//-----------------------------------------------------------------------------
//                      uHAB configuration
//-----------------------------------------------------------------------------

/** HTTP UI provider port number */
#define CFG_UHAB_UIPROVIDER_HTTP_PORT           8080
#define CFG_HTTPD_WWW_ROOT_DIR                  CFG_UHAB_ROOT_FS "/www"

/** Get request pooling timeout */
#define CFG_UHAB_UIPROVIDER_POOL_TIMEOUT        60000

#define CFG_UIPROVIDER_DEFAULT_WIFI_SSID     "suxoap"
#define CFG_UIPROVIDER_DEFAULT_WIFI_PASSWD   "kokosak123456"


/** Define HTTP connection context variables */
#define HTTPD_CON_REST_API_CONTEXT \
   int element_count;

#define CFG_HTTPD_MAXNUM_CONNECTIONS          10

/** BUS events queue size */
#define CFG_UHAB_BUS_QUEUE_SIZE           1024

/** XML parser buffer size */
#define CFG_XML_BUFSIZE                   8192

/** Max double value number */
#define CFG_MAX_DOUBLE_VALUE              100000000000.0


#define CFG_SYSTEM_CONFIG_KEY_BUS_CLICK_TMLEN                  "bus.click_timelen"
#define CFG_SYSTEM_CONFIG_KEY_BUS_DBLCLICK_TMLEN               "bus.doubleclick_timelen"
#define CFG_SYSTEM_CONFIG_KEY_BUS_LONGCLICK_TMLEN              "bus.longclick_timelen"
#define CFG_SYSTEM_CONFIG_KEY_BUS_LONGPRESS_TMLEN              "bus.longpress_timelen"
#define CFG_SYSTEM_CONFIG_KEY_BUS_WAITCHANGES_TIMEOUT          "bus.waitstate_changes_timeout"

#define CFG_BINDING_MAXNUM_ARGS            16
#define CFG_UHAB_HTTP_QUEUE_SIZE           64

//
// Threads stack size (in double word)
//
#define CFG_INIT_THREAD_STACK_SIZE         (32 * 1024)
#define CFG_INIT_THREAD_PRIORITY           osPriorityNormal

#define CFG_BUS_THREAD_STACK_SIZE          2048
#define CFG_BUS_THREAD_PRIORITY            osPriorityNormal

#define CFG_MODBUS_THREAD_STACK_SIZE       2048
#define CFG_MODBUS_THREAD_PRIORITY         osPriorityNormal

#define CFG_DMX_THREAD_STACK_SIZE          2048
#define CFG_DMX_THREAD_PRIORITY            osPriorityNormal

#define CFG_SNMP_THREAD_STACK_SIZE         2048
#define CFG_SNMP_THREAD_PRIORITY           osPriorityNormal

#define CFG_HTTP_THREAD_STACK_SIZE         1024
#define CFG_HTTP_THREAD_PRIORITY           osPriorityNormal

#define CFG_HTTPD_THREAD_STACK_SIZE        4096
#define CFG_HTTPD_THREAD_PRIORITY          osPriorityNormal

#define CFG_MINING_THREAD_STACK_SIZE       2048
#define CFG_MINING_THREAD_PRIORITY         osPriorityNormal



//-----------------------------------------------------------------------------
//                        uHAB binding interface
//-----------------------------------------------------------------------------

#define CFG_MODBUS_BINDING_NAME           "modbus"
#define CFG_SYSTEM_BINDING_NAME           "system"
#define CFG_DMX_BINDING_NAME              "dmx"
#define CFG_VEHABUS_BINDING_NAME          "vehabus"
#define CFG_SNMP_BINDING_NAME             "snmp"
#define CFG_HTTP_BINDING_NAME             "http"
#define CFG_MINING_BINDING_NAME           "mining"

#define CFG_MAXNUM_ITEMS_GROUPS           64
#define CFG_UHAB_SYSTEM_ITEM_NAME         "system"


//-----------------------------------------------------------------------------
//                               uHAB filesystem
//-----------------------------------------------------------------------------

#define CFG_UHAB_SERVICES_CFG_FILENAME    CFG_UHAB_ROOT_FS "/conf/services/%s.cfg"

#define CFG_UHAB_ITEMS_CFG_DIR            CFG_UHAB_ROOT_FS "/conf/items"
#define CFG_UHAB_ITEMS_CFG_FILENAME       CFG_UHAB_ROOT_FS "/conf/items/%s.items"

#define CFG_UHAB_RULES_CFG_DIR            CFG_UHAB_ROOT_FS "/conf/rules"
#define CFG_UHAB_RULES_CFG_FILENAME       CFG_UHAB_RULES_CFG_DIR "/%s.rules"
#define CFG_UHAB_RULES_JSCRIPT_FILENAME   "/tmp/uhab-rules.js"

#define CFG_UHAB_SITEMAP_CFG_DIR          CFG_UHAB_ROOT_FS "/conf/sitemaps"
#define CFG_UHAB_SITEMAP_CFG_FILENAME     CFG_UHAB_ROOT_FS "/conf/sitemaps/%s.sitemap"

#define CFG_UHAB_UPGRADE_FILENAME         CFG_UHAB_ROOT_FS "/upgrade.pkg"
#define CFG_UHAB_BACKUP_FILENAME          CFG_UHAB_ROOT_FS  "/backup.pkg"

#define CFG_UHAB_ICONS_DIR                CFG_UHAB_ROOT_FS "/icons"
#define CFG_UHAB_ICON_FILENAME            CFG_UHAB_ICONS_DIR "/%s.png"

#define CFG_SYSTEM_NETWORK_CFG_FILENAME   "network_interfaces"

#define CFG_ENABLE_ROMFS                  1

//-----------------------------------------------------------------------------
//                               uHAB log system
//-----------------------------------------------------------------------------

#define CFG_FSLOG_NAME                    "/tmp/uhab"
#define CFG_FSLOG_NUM_ROTATE_FILES        4
#define CFG_FSLOG_ROTATE_FILESIZE         (1024 * 1024)


#endif   // __CONFIG_H
