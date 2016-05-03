


/***

History:
2016-04-23: Ted: Create

*/

#include "AaSysCom.h"
#include "AaSysLog.h"
#include <stdbool.h>
#include "ApiInternalMsg.h"


typedef struct SMsgInternalHeader_t {
    SAaSysComMsgId  msg_id;
    SAaSysComSicad  target;
    SAaSysComSicad  sender;
    u16             body_size;
    void*           body;
} SMsgInternalHeader;


#define MSGQUEUE_NAME_MAXLENGTH     16

#define MSGQUEUE_MAXDEEP    8


static mico_queue_t msg_queue[MsgQueue_MAX] = {NULL};


static char* AaSysComPrintThreadName(SAaSysComSicad t_id);


void AaSysComInit(void)
{
    OSStatus err;
    char queue_name[MSGQUEUE_NAME_MAXLENGTH];

    for(u8 i=0; i<MsgQueue_MAX; i++) {
        sprintf(queue_name, "MsgQueue%d", i + 1);
        err = mico_rtos_init_queue(&msg_queue[i], queue_name, sizeof(SMsgInternalHeader), MSGQUEUE_MAXDEEP);
        if(err != kNoErr) {
            AaSysLogPrint(LOGLEVEL_ERR, "MsgQueue%d initialize failed", i + 1);
        } else {
            AaSysLogPrint(LOGLEVEL_INF, "MsgQueue%d initialize success", i + 1);
        }
    }
}


void* AaSysComCreate(SAaSysComMsgId msgid, SAaSysComSicad sender, SAaSysComSicad receiver, u16 pl_size)
{
    if(sender >= MsgQueue_MAX || receiver >= MsgQueue_MAX) {
        AaSysLogPrint(LOGLEVEL_ERR, "sender 0x%02x or receiver 0x%02x failed", sender, receiver);
        return NULL;
    }

    if(msgid >= API_MESSAGE_ID_MAX) {
        AaSysLogPrint(LOGLEVEL_ERR, "message id 0x%04x failed", msgid);
        return NULL;
    }

    SMsgHeader* msg_ptr = malloc(sizeof(SMsgHeader) + pl_size);
    if(msg_ptr == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "get payload buffer failed");
        return NULL;
    }

    msg_ptr->msg_id = msgid;
    msg_ptr->target = receiver;
    msg_ptr->sender = sender;
    msg_ptr->pl_size = pl_size;

    AaSysLogPrint(LOGLEVEL_DBG, "create message 0x%04x success", msgid);

    return (void*)msg_ptr;
}


void* AaSysComGetPayload(void* msg_ptr)
{
    u8* msg = msg_ptr;
    return (msg + sizeof(SMsgHeader));
}


OSStatus AaSysComSend(void* msg_ptr)
{
    SMsgInternalHeader header;
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;

    if(msg->sender >= MsgQueue_MAX || msg->target >= MsgQueue_MAX) {
        AaSysLogPrint(LOGLEVEL_ERR, "sender 0x%02x or receiver 0x%02x failed", 
                msg->sender, msg->target);
        return kParamErr;
    }

    if(msg->msg_id >= API_MESSAGE_ID_MAX) {
        AaSysLogPrint(LOGLEVEL_ERR, "message id 0x%04x failed", msg->msg_id);
        return kParamErr;
    }

    header.msg_id = msg->msg_id;
    header.target = msg->target;
    header.sender = msg->sender;
    header.body_size = sizeof(SMsgHeader) + msg->pl_size;
    header.body = msg_ptr;

    if(kNoErr != mico_rtos_push_to_queue(&msg_queue[header.target], &header, MICO_WAIT_FOREVER)) {
        AaSysLogPrint(LOGLEVEL_ERR, "message 0x%04x send failed", header.msg_id);

        if(msg_ptr != NULL) free(msg_ptr);
        return kInProgressErr;
    }

    AaSysLogPrint(LOGLEVEL_DBG, "message(0x%04x) have been sent from %s to %s", 
            header.msg_id, 
            AaSysComPrintThreadName(header.sender), 
            AaSysComPrintThreadName(header.target));

    return kNoErr;
}


void* AaSysComReceiveHandler(SAaSysComSicad receiver, u32 timeout)
{
    bool failed = false;
    SMsgInternalHeader header;

    if(kNoErr == mico_rtos_pop_from_queue(&msg_queue[receiver], &header, timeout)) {
        if(header.msg_id >= API_MESSAGE_ID_MAX) {
            AaSysLogPrint(LOGLEVEL_ERR, "receive message id 0x%04x failed", header.msg_id);
            failed = true;
        }

        if(header.target >= MsgQueue_MAX || header.sender >= MsgQueue_MAX) {
            AaSysLogPrint(LOGLEVEL_ERR, "receive target 0x%02x or sender 0x%02x failed", header.target, header.sender);
            failed = true;
        }

        SMsgHeader* msg_ptr = header.body;
        if(msg_ptr == NULL) {
            AaSysLogPrint(LOGLEVEL_ERR, "receive messgae body is NULL");
            failed = true;
        }
        else if(header.body_size != (sizeof(SMsgHeader) + msg_ptr->pl_size)) {
            AaSysLogPrint(LOGLEVEL_ERR, "receive message body size %d is not match sizeof(SMsgHeader) %d + pl_size %d", 
                    header.body_size, sizeof(SMsgHeader), msg_ptr->pl_size);
            failed = true;
        }

        if(failed == true) {
            if(header.body != NULL) free(header.body);
            return NULL;
        }

        AaSysLogPrint(LOGLEVEL_DBG, "receive message id 0x%04x from %s success", 
                header.msg_id, 
                AaSysComPrintThreadName(header.sender));
        
        return (void*)header.body;
    }
    else {
        return NULL;
    }
}


OSStatus AaSysComDestory(void* msg_ptr)
{
    if(msg_ptr != NULL) free(msg_ptr);
    return kNoErr;
}

static char* AaSysComPrintThreadName(SAaSysComSicad t_id)
{
    switch(t_id) {
        case MsgQueue_DownStream: return "DownStream\0";
        case MsgQueue_DeviceHandler: return "DeviceHandler\0";
        case MsgQueue_MusicHandler: return "MusicHandler\0";
        case MsgQueue_ControllerBus: return "ControllerBus\0";
        default: return "Unknow\0";
    }
}

// end of file


