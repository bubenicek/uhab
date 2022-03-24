
#ifndef __EBUS_BINDING_H
#define __EBUS_BINDING_H

#ifndef CFG_EBUS_BINDING_NAME
#define CFG_EBUS_BINDING_NAME           "ebus"
#endif

#ifndef CFG_EBUS_THREAD_STACK_SIZE
#define CFG_EBUS_THREAD_STACK_SIZE       1024
#endif

#ifndef CFG_EBUS_THREAD_PRIORITY
#define CFG_EBUS_THREAD_PRIORITY         osPriorityNormal
#endif

#ifndef CFG_EBUS_MAXNUM_ARGS
#define CFG_EBUS_MAXNUM_ARGS          8
#endif

#ifndef CFG_EBUS_QUEUE_SIZE
#define CFG_EBUS_QUEUE_SIZE           16
#endif

#ifndef CFG_EBUS_POLL_TIME
#define CFG_EBUS_POLL_TIME            1000
#endif


#endif // __EBUS_BINDING_H
