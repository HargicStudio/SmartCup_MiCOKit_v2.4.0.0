


/***

History:
2016-03-19: Ted: Create

*/

#ifndef _BATTERY_H
#define _BATTERY_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "Object_int.h"
#include "stdbool.h"
#include "mico.h"



bool BatteryInit(void);
bool GetBatteryVoltage(float* voltage);


   
#ifdef __cplusplus
}
#endif

#endif // _BATTERY_H

// end of file


