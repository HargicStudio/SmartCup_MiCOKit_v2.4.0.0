


/***

History:
2016-04-30: Ted: Create

*/

#ifndef _AASYSCOM_H_
#define _AASYSCOM_H_

#ifdef __cplusplus
 extern "C" {
#endif 


#include "AaPlatform.h"


typedef u32 SAaSysComMsgId;
typedef u16 SAaSysComSicad;


typedef struct SMsgHeader_t {
    SAaSysComMsgId  msg_id;
    SAaSysComSicad  target;
    SAaSysComSicad  sender;
    u16             pl_size;
} SMsgHeader;


enum {
    MsgQueue_DownStream,
    MsgQueue_DeviceHandler,
    MsgQueue_MusicHandler,
    MsgQueue_HealthHandler,
    MsgQueue_ControllerBus,
    MsgQueue_MAX,
    MsgQueue_Unknow,
};


void AaSysComInit(void);
void* AaSysComCreate(SAaSysComMsgId msgid, SAaSysComSicad sender, SAaSysComSicad receiver, u16 pl_size);
void* AaSysComGetPayload(void* msg_ptr);
OSStatus AaSysComSend(void* msg_ptr);
SAaSysComSicad AaSysComGetSender(void* msg_ptr);
OSStatus AaSysComSetSender(void* msg_ptr, SAaSysComSicad sender);
SAaSysComSicad AaSysComGetReceiver(void* msg_ptr);
OSStatus AaSysComSetReceiver(void* msg_ptr, SAaSysComSicad receiver);
OSStatus AaSysComForward(void* msg_ptr, SAaSysComSicad sender, SAaSysComSicad receiver);
void* AaSysComReceiveHandler(SAaSysComSicad receiver, u32 timeout);
OSStatus AaSysComDestory(void* msg_ptr);

   
#ifdef __cplusplus
}
#endif

#endif // _AASYSCOM_H_

// end of file


