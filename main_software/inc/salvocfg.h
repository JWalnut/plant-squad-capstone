//Project: Clean Slate

#include <msp430.h>

#define OSENABLE_TASKS        TRUE
#define OSUSE_LIBRARY         TRUE
#define OSLIBRARY_TYPE        OSL
#define OSLIBRARY_CONFIG      OST

#define OSEVENTS              1
#define OSEVENT_FLAGS         0
#define OSMESSAGE_QUEUES      0
#define OSTASKS               2
