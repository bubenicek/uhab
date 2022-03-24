
#ifndef __LINUX_BOARD_H
#define __LINUX_BOARD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <poll.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>


#define CFG_HW_VERSION_MODEL        1
#define CFG_HW_VERSION_REVISION     0

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
// uHAB platform dependencies
//
#define CFG_VEHABUS_ENABLED                  1
#define CFG_SNMP_ENABLED                     1

//
// Board configuration
//

#define LED_SYSTEM            HAL_LED0
#define CFG_HAL_LED_DEF       {}

/** UART definition */
#define CFG_HAL_UART_DEF { \
   {"/dev/ttyS4"}, /* RS485_1 */ \
   {"/dev/ttyS3"}, /* RS485_2 */ \
   {"/dev/ttyS2"}, /* RS485_3 */ \
   {"/dev/ttyS0"}, /* RS485_4 */ \
}

/** GPIO definition */
#define CFG_HAL_GPIO_DEF { \
/* GPIO_00 */  {SUNXI_GPB(12), SUNXI_GPIO_OUTPUT, 0},  /* LED1 */ \
/* GPIO_01 */  {SUNXI_GPB(19), SUNXI_GPIO_OUTPUT, 0},  /* LED2 */ \
/* GPIO_02 */  {SUNXI_GPB(21), SUNXI_GPIO_OUTPUT, 0},  /* LED3 */ \
/* GPIO_03 */  {SUNXI_GPH(8),  SUNXI_GPIO_INPUT, 0},   /* SWT */ \
/* GPIO_04 */  {SUNXI_GPD(9),  SUNXI_GPIO_INPUT, 0},   /* SWL */ \
/* GPIO_05 */  {SUNXI_GPD(13), SUNXI_GPIO_INPUT, 0},   /* SWR */ \
/* GPIO_06 */  {SUNXI_GPD(11), SUNXI_GPIO_INPUT, 0},   /* SWB */ \
/* GPIO_07 */  {SUNXI_GPB(18), SUNXI_GPIO_INPUT, 0},   /* JOYSTICK_SWITCH */ \
/* GPIO_08 */  {SUNXI_GPE(5),  SUNXI_GPIO_OUTPUT, 0},   /* RS485_1_DIR */ \
/* GPIO_09 */  {SUNXI_GPE(7),  SUNXI_GPIO_OUTPUT, 0},   /* RS485_2_DIR */ \
/* GPIO_10 */  {SUNXI_GPE(9),  SUNXI_GPIO_OUTPUT, 0},   /* RS485_3_DIR */ \
/* GPIO_11 */  {SUNXI_GPF(5),  SUNXI_GPIO_INPUT, 0},   /* ZW_P3.7 */ \
/* GPIO_12 */  {SUNXI_GPF(3),  SUNXI_GPIO_INPUT, 0},   /* ZW_P3.6 */ \
/* GPIO_13 */  {SUNXI_GPF(0),  SUNXI_GPIO_INPUT, 0},   /* ZW_P1.1 */ \
/* GPIO_14 */  {SUNXI_GPF(1),  SUNXI_GPIO_OUTPUT, 0},  /* ZW_RST */ \
/* GPIO_15 */  {SUNXI_GPF(2),  SUNXI_GPIO_OUTPUT, 1},  /* USB power ON */ \
/* GPIO_16 */  {SUNXI_GPD(1),  SUNXI_GPIO_OUTPUT, 0},  /* OC_OUT1 */ \
/* GPIO_17 */  {SUNXI_GPD(3),  SUNXI_GPIO_OUTPUT, 0},  /* OC_OUT2 */ \
/* GPIO_18 */  {SUNXI_GPD(5),  SUNXI_GPIO_OUTPUT, 0},  /* OC_OUT3 */ \
/* GPIO_19 */  {SUNXI_GPD(7),  SUNXI_GPIO_OUTPUT, 0},  /* OC_OUT4 */ \
/* GPIO_20 */  {SUNXI_GPB(9),  SUNXI_GPIO_INPUT, 0},   /* OC_IN1 */ \
/* GPIO_21 */  {SUNXI_GPB(11), SUNXI_GPIO_INPUT, 0},   /* OC_IN2 */ \
/* GPIO_22 */  {SUNXI_GPB(5),  SUNXI_GPIO_INPUT, 0},   /* OC_IN3 */ \
/* GPIO_23 */  {SUNXI_GPB(7),  SUNXI_GPIO_INPUT, 0},   /* OC_IN4 */ \
/* GPIO_24 */  {SUNXI_GPD(27), SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K1 */ \
/* GPIO_25 */  {SUNXI_GPD(26), SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K2 */ \
/* GPIO_26 */  {SUNXI_GPD(25), SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K3 */ \
/* GPIO_27 */  {SUNXI_GPD(24), SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K4 */ \
/* GPIO_28 */  {SUNXI_GPD(22), SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K5 */ \
/* GPIO_29 */  {SUNXI_GPD(2),  SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K6 */ \
/* GPIO_30 */  {SUNXI_GPD(0),  SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K7 */ \
/* GPIO_31 */  {SUNXI_GPH(2),  SUNXI_GPIO_OUTPUT, 0},  /* RELAY_K8 */ \
/* GPIO_32 */  {SUNXI_GPD(18), SUNXI_GPIO_OUTPUT, 0},  /* RJ.PWR.PER1 */ \
/* GPIO_33 */  {SUNXI_GPD(20), SUNXI_GPIO_OUTPUT, 0},  /* RJ.PWR.PER2 */ \
/* GPIO_34 */  {SUNXI_GPH(22), SUNXI_GPIO_INPUT, 0},   /* RJ1.1 */ \
/* GPIO_35 */  {SUNXI_GPH(24), SUNXI_GPIO_INPUT, 0},   /* RJ1.2 */ \
/* GPIO_36 */  {SUNXI_GPH(26), SUNXI_GPIO_INPUT, 0},   /* RJ1.3 */ \
/* GPIO_37 */  {SUNXI_GPH(3),  SUNXI_GPIO_INPUT, 0},   /* RJ1.6 */ \
/* GPIO_38 */  {SUNXI_GPH(9),  SUNXI_GPIO_INPUT, 0},   /* RJ2.1 */ \
/* GPIO_39 */  {SUNXI_GPH(11), SUNXI_GPIO_INPUT, 0},   /* RJ2.2 */ \
/* GPIO_40 */  {SUNXI_GPH(13), SUNXI_GPIO_INPUT, 0},   /* RJ2.3 */ \
/* GPIO_41 */  {SUNXI_GPH(15), SUNXI_GPIO_INPUT, 0},   /* RJ2.6 */ \
/* GPIO_42 */  {SUNXI_GPH(17), SUNXI_GPIO_INPUT, 0},   /* RJ3.1 */ \
/* GPIO_43 */  {SUNXI_GPH(19), SUNXI_GPIO_INPUT, 0},   /* RJ3.2 */ \
/* GPIO_44 */  {SUNXI_GPH(21), SUNXI_GPIO_INPUT, 0},   /* RJ3.3 */ \
/* GPIO_45 */  {SUNXI_GPH(23), SUNXI_GPIO_INPUT, 0},   /* RJ3.6 */ \
/* GPIO_46 */  {SUNXI_GPH(18), SUNXI_GPIO_INPUT, 0},   /* RJ4.1 */ \
/* GPIO_47 */  {SUNXI_GPH(20), SUNXI_GPIO_INPUT, 0},   /* RJ4.2 */ \
/* GPIO_48 */  {SUNXI_GPH(27), SUNXI_GPIO_INPUT, 0},   /* RJ4.3 */ \
/* GPIO_49 */  {SUNXI_GPH(25), SUNXI_GPIO_INPUT, 0},   /* RJ4.6 */ \
/* GPIO_50 */  {SUNXI_GPD(23), SUNXI_GPIO_INPUT, 0},   /* RJ5.1 */ \
/* GPIO_51 */  {SUNXI_GPD(21), SUNXI_GPIO_INPUT, 0},   /* RJ5.2 */ \
/* GPIO_52 */  {SUNXI_GPD(19), SUNXI_GPIO_INPUT, 0},   /* RJ5.3 */ \
/* GPIO_53 */  {SUNXI_GPD(17), SUNXI_GPIO_INPUT, 0},   /* RJ5.6 */ \
/* GPIO_54 */  {SUNXI_GPD(10), SUNXI_GPIO_INPUT, 0},   /* RJ6.1 */ \
/* GPIO_55 */  {SUNXI_GPD(12), SUNXI_GPIO_INPUT, 0},   /* RJ6.2 */ \
/* GPIO_56 */  {SUNXI_GPD(14), SUNXI_GPIO_INPUT, 0},   /* RJ6.3 */ \
/* GPIO_57 */  {SUNXI_GPD(16), SUNXI_GPIO_INPUT, 0},   /* RJ6.4 */ \
/* GPIO_58 */  {SUNXI_GPD(15), SUNXI_GPIO_INPUT, 0},   /* RJ7.1 */ \
/* GPIO_59 */  {SUNXI_GPD(4),  SUNXI_GPIO_INPUT, 0},   /* RJ7.2 */ \
/* GPIO_60 */  {SUNXI_GPD(6),  SUNXI_GPIO_INPUT, 0},   /* RJ7.3 */ \
/* GPIO_61 */  {SUNXI_GPD(8),  SUNXI_GPIO_INPUT, 0},   /* RJ7.6 */ \
/* GPIO_62 */  {SUNXI_GPH(16), SUNXI_GPIO_INPUT, 0},   /* RJ8.1 */ \
/* GPIO_63 */  {SUNXI_GPH(14), SUNXI_GPIO_INPUT, 0},   /* RJ8.2 */ \
/* GPIO_64 */  {SUNXI_GPH(12), SUNXI_GPIO_INPUT, 0},   /* RJ8.3 */ \
/* GPIO_65 */  {SUNXI_GPH(10), SUNXI_GPIO_INPUT, 0},   /* RJ8.6 */ \
}

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
#define VEHABUS_GPIO_NUM               32

#define VEHABUS_GPIO_PWR1              HAL_GPIO32
#define VEHABUS_GPIO_PWR2              HAL_GPIO33

#define VEHABUS_LED_GPIO_NUM           (VEHABUS_GPIO_NUM * 2)

#define VEHABUS_GPIO_RELAY_START       HAL_GPIO24
#define VEHABUS_GPIO_RELAYS_NUM        8

#define VEHABUS_OC_OUTPUT_GPIO_START   HAL_GPIO16
#define VEHABUS_OC_OUTPUT_GPIO_NUM     4              // Number of open colector outputs

#define VEHABUS_OC_INPUT_GPIO_NUM      4              // Number of optoisolated inputs
#define VEHABUS_ADC_NUM                2              // Number of ADC

//
// RS485
//
#define RS485_1_GPIO_DIR               HAL_GPIO08
#define RS485_2_GPIO_DIR               HAL_GPIO09
#define RS485_3_GPIO_DIR               HAL_GPIO10

//
// DMX512
//
#define CFG_DMX_MARK_AFTER_BREAK_DELAY    20000

//
// Network interface
//
#define CFG_NETIF                         HAL_NETIF_ETH

//
// Filesystem
//
#define CFG_UHAB_ROOT_FS                  "."

// UHAB log filesystem
#define CFG_FSLOG_ENABLED                 1


#endif   // __LINUX_BOARD_H
