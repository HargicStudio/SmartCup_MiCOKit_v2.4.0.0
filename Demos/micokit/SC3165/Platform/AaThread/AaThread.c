


/***

History:
2016-04-23: Ted: Create

*/

#include "AaThread.h"
#include "AaSysLog.h"
#include "AaInt.h"
#include "stdbool.h"
#include "include_for_mico.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("LightsMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("LightsMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("LightsMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


EStatus AaThreadCreate()
{
    
}




// end of file


