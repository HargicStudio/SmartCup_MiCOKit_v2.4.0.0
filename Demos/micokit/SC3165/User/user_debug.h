
/***

History:
2015-09-30: Ted: Create

*/

#ifndef _USER_DEBUG_H
#define _USER_DEBUG_H

#ifdef __cplusplus
 extern "C" {
#endif 


#include "MICO.h"


#ifdef USER_DEBUG

extern mico_mutex_t user_debug_mutex;

// N: DBG/INF/WRN/ERR
#define user_debug(N, M, ...)    do { \
                                    mico_rtos_lock_mutex(&user_debug_mutex); \
                                    printf("[%d][%s] " M "\r\n", mico_get_time(), N, ##__VA_ARGS__); \
                                    mico_rtos_unlock_mutex(&user_debug_mutex); \
                                }while(0)

#endif // USER_DEBUG


/* User defined debug log functions
 */
#define user_err_log(M, ...) user_debug("ERR", M, ##__VA_ARGS__)
#define user_wrn_log(M, ...) user_debug("WRN", M, ##__VA_ARGS__)
#define user_inf_log(M, ...) user_debug("INF", M, ##__VA_ARGS__)
#define user_dbg_log(M, ...) user_debug("DBG", M, ##__VA_ARGS__)


void UserDebug_Init();



#ifdef __cplusplus
}
#endif

#endif // _USER_DEBUG_H

// end of file