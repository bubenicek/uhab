/**
 * \file uhab.h         \brief uHAB server common definitions 
 */
 
#ifndef __UHAB_H
#define __UHAB_H

#include "system.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "repository/repository.h"
#include "binding/binding.h"
#include "bus.h"
#include "automation/automation.h"
#include "uiprovider/uiprovider.h"
#include "uhab_config.h"


//
// System module initializatin flags
//
#define SYSTEM_INIT_CONFIG             0x01
#define SYSTEM_INIT_BUS_FLAG           0x02
#define SYSTEM_INIT_BINDING_FLAG       0x04
#define SYSTEM_INIT_REPOSITORY_FLAG    0x08
#define SYSTEM_INIT_AUTOMATION_FLAG    0x10
#define SYSTEM_INIT_UIPROVIDER_FLAG    0x20
#define SYSTEM_START_BINDING_FLAG      0x40

/** All modules are initialized */
#define SYSTEM_READY_FLAGS             0x7F

/** Initialize system module always */
#define SYSTEM_INIT(_func, _flag) do {       \
      if (_func == 0)                        \
         system_status.init_flags |= _flag;  \
      else                                   \
         res--;                              \
} while(0)

/** Initialize system module only when previous module was successfull initialized */
#define VERIFY_SYSTEM_INIT(_func, _flag) do {   \
   if (res == 0) {                              \
      if (_func == 0)                           \
         system_status.init_flags |= _flag;     \
      else                                      \
         res--;                                 \
   }                                            \
} while(0)


/** System status information */
typedef struct
{
   uint32_t init_flags;
   
} uhab_system_status_t;

//
// Globals
//
extern uhab_system_status_t   system_status;
extern uhab_repository_t      repository;
extern uhab_automation_t      automation;    
extern uhab_uiprovider_t      uiprovider;   


/** Shutdown system */
void uhab_system_shutdown(void);

/** Restart system */
void uhab_system_restart(void);

/** Reset system to defaults */
void uhab_system_factory_reset(void);


#endif // __UHAB_H
