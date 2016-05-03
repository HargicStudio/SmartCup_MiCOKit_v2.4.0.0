


/***

History:
2016-04-30: Ted: Create

*/

#ifndef _AASYSLOG_H_
#define _AASYSLOG_H_

#ifdef __cplusplus
 extern "C" {
#endif 


#include "AaPlatform.h"
    

enum {
    LOGLEVEL_CLS = 0,
    LOGLEVEL_ERR,
    LOGLEVEL_WRN,
    LOGLEVEL_INF,
    LOGLEVEL_DBG,
    LOGLEVEL_ALL,
};


extern mico_mutex_t stdio_tx_mutex;
extern uint8_t _syslog_level;


#define AaSysLogPrint(level, M, ...) do { \
                                    if(level > _syslog_level) break; \
                                    mico_rtos_lock_mutex( &stdio_tx_mutex ); \
                                    printf("[%d]", mico_get_time()); \
                                    switch(level) { \
                                        case LOGLEVEL_ERR: printf("[ERR]"); break; \
                                        case LOGLEVEL_WRN: printf("[WRN]"); break; \
                                        case LOGLEVEL_INF: printf("[INF]"); break; \
                                        case LOGLEVEL_DBG: printf("[DBG]"); break; \
                                        default: printf("[NC]"); break; \
                                    } \
                                    printf("[%s:%4d] " M "\r\n", SHORT_FILE, __LINE__, ##__VA_ARGS__); \
                                    mico_rtos_unlock_mutex( &stdio_tx_mutex ); \
                                } while(0)


void AaSysLogInit(void);

   
#ifdef __cplusplus
 }
#endif

#endif // _AASYSLOG_H_

// end of file


