
#ifndef __LINUX_BOARD_H
#define __LINUX_BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <pthread.h>


#define CFG_HW_VERSION_MODEL        1
#define CFG_HW_VERSION_REVISION     0

#define ENABLE_INTERRUPTS()
#define DISABLE_INTERRUPTS()

#define VERIFY_FATAL(EX) {             \
 if (!(EX))                                                               \
 {                                                                        \
    TRACE_ERROR("(%s) Fatal error at %s:%d", #EX, __FUNCTION__, __LINE__);\
    exit(1); \
 } \
}

#define ASSERT(EX)                                                             \
 if (!(EX))                                                                    \
 {                                                                             \
    TRACE_ERROR("(%s) Assert failed at %s:%d", #EX, __FUNCTION__, __LINE__);   \
    exit(1); \
 }

#define closesocket  close

//
// Board configuration
//
#define CFG_SNMP_ENABLED                     1

#define LED_SYSTEM            HAL_LED0


#define CFG_HAL_LED_DEF  {}

#define CFG_HAL_GPIO_DEF {}

#define CFG_HAL_UART_DEF { \
   {"/dev/ttyUSB0"}, \
   {"/dev/ttyUSB1"}, \
   {"/dev/ttyUSB2"}, \
   {"/dev/ttyUSB3"}, \
}

//
// LEDs
//
#define LED_ERROR                      0
#define GPIO_LED_DMX_POLL              HAL_GPIO1
#define GPIO_LED_DMX_FADER             HAL_GPIO2
#define GPIO_DEBUG                     HAL_GPIO16


//
// Network interface
//
#define CFG_NETIF                      HAL_NETIF_ETH
#define CFG_HAL_NETIF_ETH_NAME        "enp0s25"

//#define CFG_NETIF                      HAL_NETIF_WIFI
//#define CFG_HAL_NETIF_WIFI_NAME        "wlp3s0"

//
// Filesystem
//
#define CFG_UHAB_ROOT_FS                  "."

// UHAB log filesystem
#define CFG_FSLOG_ENABLED                 1




#endif   // __LINUX_BOARD_H
