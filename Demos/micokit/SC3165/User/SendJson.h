


/***

History:
2015-12-22: Ted: Create

*/

#ifndef _SENDJSON_H
#define _SENDJSON_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "Object_int.h"
#include "stdbool.h"
#include "MicoFogCloud.h"


bool SendJsonInt(app_context_t *arg, char* str, int value);
bool SendJsonDouble(app_context_t *arg, char* str, double value);
bool SendJsonBool(app_context_t *arg, char* str, bool value);
bool SendJsonLedConf(app_context_t *arg);
bool SendJsonNoDisturbingConf(app_context_t *arg);
bool SendJsonPickup(app_context_t *arg);
bool SendJsonPutdown(app_context_t *arg);
bool SendJsonImmediate(app_context_t *arg);
bool SendJsonSchedule(app_context_t *arg);
bool SendJsonAppointment(app_context_t *arg);
bool SendJsonTrack(app_context_t *arg);

   
#ifdef __cplusplus
}
#endif

#endif // _SENDJSON_H

// end of file


