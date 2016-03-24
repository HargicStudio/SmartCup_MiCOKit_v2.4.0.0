


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


// Battery voltage range
// unit: 10mV
#define BATTERY_VOLTAGE_LOW     (250.0f)
#define BATTERY_VOLTAGE_HIGH    (425.0f)


bool BatteryInit(void);
bool GetBatteryVoltage(float* voltage);


   
#ifdef __cplusplus
}
#endif

#endif // _BATTERY_H

// end of file


