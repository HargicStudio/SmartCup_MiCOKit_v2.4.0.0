


/***

History:
2016-03-14: Ted: Create

*/

#ifndef _OUTTRIGGER_H
#define _OUTTRIGGER_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "Object_int.h"
#include "stdbool.h"
#include "mico.h"


typedef enum EKey_t {
    OUTERTRIGGER_PICKUP = 0,
    OUTERTRIGGER_PUTDOWN,
    OUTERTRIGGER_ERR,
    OUTERTRIGGER_NONE
} EKey;


void OuterTriggerInit(mico_gpio_irq_handler_t handler);
EKey GetOuterTriggerStatus(void);

   
#ifdef __cplusplus
}
#endif

#endif // _OUTTRIGGER_H

// end of file


