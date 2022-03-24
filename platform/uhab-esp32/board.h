
#ifndef __ESP32_UHAB_BOARD_H
#define __ESP32_UHAB_BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"


#define CFG_HW_VERSION_MODEL        1
#define CFG_HW_VERSION_REVISION     0


/** Application entry point function */
#define APPLICATION_MAIN()  void app_main(void)

/** Trace float formating */
#define CFG_TRACE_HAS_CVTBUF_FUNCTIONS          1

#define CFG_ESP32_HEAP_SIZE                     (256 * 1024)

/** FreeRTOS missing defines */
#define portEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != pdFALSE ) portYIELD()
static inline size_t xPortGetTotalHeapSize(void)
{
   return CFG_ESP32_HEAP_SIZE;
}

//
// uHAB platform dependencies
//
#define CFG_VEHABUS_ENABLED                     1
#define CFG_SNMP_ENABLED                        0

/** LED defs */
#define CFG_HAL_LED_DEF  {}

/** UART definition */
#define CFG_HAL_UART_DEF {}

/** GPIO definition */
#define CFG_HAL_GPIO_DEF {}


//
// Board configuration
//
#define LED_SYSTEM                              HAL_LED0
#define LED_ERROR                               HAL_LED0

//
// Buttons
//
#define GPIO_BTN_TOP                   HAL_GPIO3
#define GPIO_BTN_LEFT                  HAL_GPIO4
#define GPIO_BTN_RIGHT                 HAL_GPIO5
#define GPIO_BTN_BOTTOM                HAL_GPIO6

//
// LEDs
//
#define GPIO_LED_DMX_POLL              HAL_GPIO1
#define GPIO_LED_DMX_FADER             HAL_GPIO2
#define GPIO_DEBUG                     HAL_GPIO16


//
// VEHABUS
//
#define VEHABUS_GPIO_START             HAL_GPIO34
#define VEHABUS_GPIO_NUM               64

#define VEHABUS_GPIO_PWR1              HAL_GPIO32
#define VEHABUS_GPIO_PWR2              HAL_GPIO33

#define VEHABUS_GPIO_RELAY_START       HAL_GPIO24
#define VEHABUS_GPIO_RELAYS_NUM        8

#define VEHABUS_OC_OUTPUT_GPIO_START   HAL_GPIO16
#define VEHABUS_OC_OUTPUT_GPIO_NUM     4              // Number of open colector outputs

#define VEHABUS_OC_INPUT_GPIO_NUM      4              // Number of optoisolated inputs
#define VEHABUS_ADC_NUM                2              // Number of ADC

#define VEHABUS_LED_GPIO_NUM           VEHABUS_GPIO_NUM

//
// RS485
//
#define RS485_1_GPIO_DIR               HAL_GPIO08
#define RS485_2_GPIO_DIR               HAL_GPIO09
#define RS485_3_GPIO_DIR               HAL_GPIO10

//
// Network interface
//
#define CFG_NETIF                      HAL_NETIF_WIFI

//
// Filesystem
//
#define CFG_UHAB_ROOT_FS               "/spiffs"
#define CFG_SPIFFS_MAX_OPEN_FILES      16

// UHAB log filesystem is disabled
#define CFG_FSLOG_ENABLED               0



#endif   // __ESP32_UHAB_BOARD_H
