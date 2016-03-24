
/***

History:
2016-03-15: Ted: Create
            Transfer protocol between EMW3165 and STM32F411 base on SPI

*/

#ifndef _CONTROLLERBUS_H
#define _CONTROLLERBUS_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "Object_int.h"
#include "stdbool.h"
#include "mico.h"


typedef struct SCBusHeader_t {
    u8  magic;
    u16 datalen;
    u8  cmd;
    u8  checksum;
    u8  tail;
} SCBusHeader;

typedef enum ECBusCmd_t {
    
} ECBusCmd;


   
#ifdef __cplusplus
}
#endif

#endif // _CONTROLLERBUS_H

// end of file


