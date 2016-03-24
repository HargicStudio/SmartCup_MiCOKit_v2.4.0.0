


/***

History:
2016-03-17: Ted: Create

*/

#ifndef _TEMPERATURE_H
#define _TEMPERATURE_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "Object_int.h"
#include "stdbool.h"
#include "mico.h"



bool TemperatureInit(void);
bool TMP75ReadTemperature(float* temperature);


   
#ifdef __cplusplus
}
#endif

#endif // _TEMPERATURE_H

// end of file


