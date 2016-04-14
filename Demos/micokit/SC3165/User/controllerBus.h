
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


/*
 *  align at uint16_t, or compiller will autocomplete as uint16_t
 */
typedef struct SCBusHeader_t {
    uint8_t  magic;
    uint8_t  cmd;
    uint16_t datalen;
    uint8_t  checksum;
    uint8_t  tail;
} SCBusHeader;

typedef enum ECBusCmd_t {
    CONTROLLERBUS_CMD_TFSTATUS = 0x21,
} ECBusCmd;



bool ControllerBusInit(void);
void PinInitForUsart(void);
OSStatus ControllerBusSend(unsigned char *inBuf, unsigned int inBufLen);


   
#ifdef __cplusplus
}
#endif

#endif // _CONTROLLERBUS_H

// end of file


