
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
    CONTROLLERBUS_CMD_GETTRACKNUM   = 0x11,
    CONTROLLERBUS_CMD_GETTRQACKNAME = 0x12,
    CONTROLLERBUS_CMD_PLAY          = 0x13,
    CONTROLLERBUS_CMD_DELETE        = 0x14,
    CONTROLLERBUS_CMD_ADD           = 0x15,
    CONTROLLERBUS_CMD_EXIT          = 0x16,
    CONTROLLERBUS_CMD_VOLUME        = 0x17,
    CONTROLLERBUS_CMD_QUERYSTATUS   = 0x18,
    CONTROLLERBUS_CMD_TFSTATUS      = 0x21,
} ECBusCmd;



bool ControllerBusInit(void);
void PinInitForUsart(void);
OSStatus ControllerBusSend(ECBusCmd cmd, unsigned char *inData, unsigned int inDataLen);


   
#ifdef __cplusplus
}
#endif

#endif // _CONTROLLERBUS_H

// end of file


