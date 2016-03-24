


/***

History:
2016-03-15: Ted: Create

*/

#ifndef _LED_H
#define _LED_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "Object_int.h"
#include "stdbool.h"
#include "mico.h"



bool LedDutyInit(float r_duty, float g_duty, float b_duty);
void LedPwmStart(void);
void LedPwmStop(void);

   
#ifdef __cplusplus
}
#endif

#endif // _LED_H

// end of file


