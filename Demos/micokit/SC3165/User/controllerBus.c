
/***

History:
2016-03-15: Ted: Create

*/

#include "controllerBus.h"
#include "user_debug.h"
#include "TimeUtils.h"
#include "user_uart.h"
#include "If_MO.h"
#include "MusicMonitor.h"
#include "AaInclude.h"
#include <stdio.h>


#define user_log(M, ...) custom_log("ControllerBus", M, ##__VA_ARGS__)
#define user_log_trace() custom_log_trace("ControllerBus")



#define CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD    0x400


#define CONTROLLERBUS_MAGIC   0x5a
#define CONTROLLERBUS_TAIL    0xaa

#define F411_RESET_PIN  F411_PIN_RST



bool is_serial_data_print_open = true;

#define CONTROLLERBUS_RECV_BUFFER_LENGTH      USER_UART_BUFFER_LENGTH

uint8_t recv_buffer[CONTROLLERBUS_RECV_BUFFER_LENGTH];

static mico_thread_t bus_recv_handle = NULL;
static mico_thread_t bus_send_handle = NULL;

uint16_t g_track_num;
uint16_t g_track_query_idx;


static void ControllerBusMsgHandler(void* arg);
static OSStatus HandleTfStatusReq(void* msg_ptr);
static OSStatus HandleVolumeReq(void* msg_ptr);
static OSStatus HandleTrackNumberReq(void* msg_ptr);
static OSStatus HandleTrackNameReq(void* msg_ptr);
static OSStatus HandlePlayReq(void* msg_ptr);
static OSStatus HandleQuitReq(void* msg_ptr);
static void ControllerBusProtocolHandler(void* arg);
static OSStatus ParseControllerBus(SCBusHeader* header, uint8_t* payload);
static OSStatus ParseQuitResp(uint8_t* payload);
static OSStatus ParseTFCardStatus(uint8_t* payload);
static OSStatus ParseVolume(uint8_t* payload);
static OSStatus ParseTrackNumber(SCBusHeader* header, uint8_t* payload);
static OSStatus ParseTrackName(SCBusHeader* header, uint8_t* payload);
static OSStatus ParseTrackPlay(uint8_t* payload);
static void PinInitForUsart(void);



OSStatus ControllerBusSend(ECBusCmd cmd, unsigned char *inData, unsigned int inDataLen)
{
    unsigned char* inBuf;
    unsigned int bufLen = sizeof(SCBusHeader) + inDataLen;

    inBuf = malloc(bufLen);
    if(inBuf == NULL) {
        user_log("[ERR]ControllerBusSend: memory failed");
        return kGeneralErr;
    }

    SCBusHeader* header = (SCBusHeader*)inBuf;

    header->magic = 0x5a;
    header->cmd = cmd;
    header->datalen = inDataLen;
    header->tail = 0xaa;

    uint8_t data_checksum = 0;
    data_checksum += header->magic;
    data_checksum += header->cmd;
    data_checksum += header->datalen >> 8;
    data_checksum += header->datalen & 0x00ff;
    data_checksum += header->tail;
    for(uint16_t idx = 0; idx < inDataLen; idx++) {
        data_checksum += *(inData + idx);
    }
    header->checksum = data_checksum;

    if(inData != NULL) {
        unsigned char* payload = inBuf + sizeof(SCBusHeader);

        memcpy(payload, inData, inDataLen);
    }

    if(kNoErr != user_uartSend(inBuf, bufLen)) {
        user_log("[ERR]ControllerBusSend: send failed");
        return kGeneralErr;
    }

    user_log("[DBG]ControllerBusSend: send with following data");
    print_serial_data(inBuf, bufLen);

    if(inBuf != NULL) free(inBuf);
    
    return kNoErr;
}

static void ControllerBusMsgHandler(void* arg)
{
    void* msg_ptr;
    SMsgHeader* msg;
    
    while(1) {
        msg_ptr = AaSysComReceiveHandler(MsgQueue_ControllerBus, MICO_WAIT_FOREVER);
        msg = (SMsgHeader*)msg_ptr;

        AaSysLogPrint(LOGLEVEL_DBG, "receive message id 0x%04x", msg->msg_id);

        switch(msg->msg_id) {
            case API_MESSAGE_ID_TFSTATUS_REQ: HandleTfStatusReq(msg_ptr); break;
            case API_MESSAGE_ID_VOLUME_REQ: HandleVolumeReq(msg_ptr); break;
            case API_MESSAGE_ID_TRACKNUM_REQ: HandleTrackNumberReq(msg_ptr); break;
            case API_MESSAGE_ID_TRACKNAME_REQ: HandleTrackNameReq(msg_ptr); break;
            case API_MESSAGE_ID_PLAY_REQ: HandlePlayReq(msg_ptr); break;
            case API_MESSAGE_ID_QUIT_REQ: HandleQuitReq(msg_ptr); break;
            default: 
                AaSysLogPrint(LOGLEVEL_ERR, "no message id 0x%04x", msg->msg_id); 
                break;
        }
        
        if(msg_ptr != NULL) AaSysComDestory(msg_ptr);
    }
}

static OSStatus HandleTfStatusReq(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;

    if(msg->pl_size != sizeof(ApiTfStatusReq)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiTfStatusReq) %d", 
                msg->pl_size, sizeof(ApiTfStatusReq));
        return kInProgressErr;
    }

    ControllerBusSend(CONTROLLERBUS_CMD_TFSTATUS, NULL, 0);

    return kNoErr;
}

static OSStatus HandleVolumeReq(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiVolumeReq)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiVolumeReq) %d", 
                msg->pl_size, sizeof(ApiVolumeReq));
        return kInProgressErr;
    }

    ApiVolumeReq* pl = AaSysComGetPayload(msg_ptr);
    u8 volume = pl->volume;

    ControllerBusSend(CONTROLLERBUS_CMD_VOLUME, &volume, sizeof(volume));

    return kNoErr;
}

static OSStatus HandleTrackNumberReq(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiTrackNumReq)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiTrackNumReq) %d", 
                msg->pl_size, sizeof(ApiTrackNumReq));
        return kInProgressErr;
    }

    ApiTrackNumReq* pl = AaSysComGetPayload(msg_ptr);

    u8 type = pl->type;

    ControllerBusSend(CONTROLLERBUS_CMD_GETTRACKNUM, &type, sizeof(type));

    AaSysLogPrint(LOGLEVEL_DBG, "send TrackNum type %d request to f411 at %s", type, __FILE__);

    return kNoErr;
}

static OSStatus HandleTrackNameReq(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiTrackNameReq)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiTrackNameReq) %d", 
                msg->pl_size, sizeof(ApiTrackNameReq));
        return kInProgressErr;
    }

    ApiTrackNameReq* pl = AaSysComGetPayload(msg_ptr);

    u8 buf[sizeof(u8) + sizeof(u16)];

    *buf = pl->type;
    *(uint16_t*)(buf + sizeof(u8)) = pl->track_index;

    ControllerBusSend(CONTROLLERBUS_CMD_GETTRQACKNAME, buf, sizeof(u8) + sizeof(u16));

    return kNoErr;
}

static OSStatus HandlePlayReq(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiPlayReq)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiPlayReq) %d", 
                msg->pl_size, sizeof(ApiPlayReq));
        return kInProgressErr;
    }

    ApiPlayReq* pl = AaSysComGetPayload(msg_ptr);

    u8 buf[sizeof(u8) + sizeof(u16)];

    *buf = pl->type;
    *(u16*)(buf + sizeof(u8)) = pl->track_index;

    ControllerBusSend(CONTROLLERBUS_CMD_PLAY, buf, sizeof(u8) + sizeof(u16));

    return kNoErr;
}

static OSStatus HandleQuitReq(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiQuitReq)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiQuitReq) %d", 
                msg->pl_size, sizeof(ApiQuitReq));
        return kInProgressErr;
    }

    ControllerBusSend(CONTROLLERBUS_CMD_EXIT, NULL, 0);

    return kNoErr;
}

static void ControllerBusProtocolHandler(void* arg)
{
    // avoid compiling warning
    arg = arg;
    uint16_t received_len;

    AaSysLogPrint(LOGLEVEL_INF, "create controllerbus thread success");

    while(1) {
        received_len = user_uartRecv(recv_buffer, sizeof(SCBusHeader));
        if(received_len == 0) {
            AaSysLogPrint(LOGLEVEL_WRN, "do not received any header");
            continue;
        }

        AaSysLogPrint(LOGLEVEL_DBG, "receive header length %d", received_len);
        print_serial_data(recv_buffer, received_len);

        if(received_len != sizeof(SCBusHeader)) {
            AaSysLogPrint(LOGLEVEL_WRN, "received header length do not match");
            continue;
        }

        SCBusHeader* header = (SCBusHeader*)recv_buffer;

        // parse the header, get the data length and checksum
        if(header->magic != CONTROLLERBUS_MAGIC || header->tail != CONTROLLERBUS_TAIL) {
            AaSysLogPrint(LOGLEVEL_WRN, "magic 0x%02x or tail 0x%02x do not match",
                    header->magic, header->tail);
            continue;
        }
        
        uint16_t datalen = header->datalen;
        uint8_t checksum = header->checksum;

        AaSysLogPrint(LOGLEVEL_DBG, "get datalen %d checksum 0x%02x", datalen, checksum);

        if(datalen == 0) {
            AaSysLogPrint(LOGLEVEL_WRN, "there is no data");
            continue;
        }
        else if(datalen >= (CONTROLLERBUS_RECV_BUFFER_LENGTH - sizeof(SCBusHeader))) {
            AaSysLogPrint(LOGLEVEL_WRN, "there is too much data and receive buffer %d is overflow", 
                    CONTROLLERBUS_RECV_BUFFER_LENGTH);
            continue;
        }

        // start to receive data
        uint8_t* payload = recv_buffer + sizeof(SCBusHeader);

        received_len = user_uartRecv(payload, datalen);
        if(received_len == 0) {
            AaSysLogPrint(LOGLEVEL_WRN, "do not received any data");
            continue;
        }
        
        AaSysLogPrint(LOGLEVEL_DBG, "receive data length %d", received_len);
        print_serial_data(payload, received_len);

        if(received_len != datalen) {
            AaSysLogPrint(LOGLEVEL_WRN, "received data length do not match");
            continue;
        }

#if 1
        // check if data available
        uint8_t data_checksum = 0;
        data_checksum += header->magic;
        data_checksum += header->cmd;
        data_checksum += header->datalen >> 8;
        data_checksum += header->datalen & 0x00ff;
        data_checksum += header->tail;
        for(uint16_t idx = 0; idx < datalen; idx++) {
            data_checksum += *(payload + idx);
        }
        if(data_checksum != checksum) {
            AaSysLogPrint(LOGLEVEL_WRN, "data checksum 0x%02x do not match received checksum 0x%02x, dropping", 
                    data_checksum, checksum);
            continue;
        }
#endif

        ParseControllerBus(header, payload);

        AaSysLogPrint(LOGLEVEL_DBG, "parse package complete");
    }

    // normally should not access
    AaSysLogPrint(LOGLEVEL_ERR, "some fatal error occur, thread dead");
    mico_rtos_delete_thread(NULL);  // delete current thread
}

static OSStatus ParseControllerBus(SCBusHeader* header, uint8_t* payload)
{
    OSStatus err = kGeneralErr;
    
    switch(header->cmd) {
        case CONTROLLERBUS_CMD_GETTRACKNUM: err = ParseTrackNumber(header, payload); break;
        case CONTROLLERBUS_CMD_GETTRQACKNAME: err = ParseTrackName(header, payload); break;
        case CONTROLLERBUS_CMD_PLAY: err = ParseTrackPlay(payload); break;
        case CONTROLLERBUS_CMD_VOLUME: err = ParseVolume(payload); break;
        case CONTROLLERBUS_CMD_TFSTATUS: err = ParseTFCardStatus(payload); break;
        case CONTROLLERBUS_CMD_EXIT: err = ParseQuitResp(payload); break;
        default: user_log("[ERR]ParseControllerBus: error cmd 0x%02x", header->cmd); break;
    }

    return err;
}

static OSStatus ParseQuitResp(uint8_t* payload)
{
    uint8_t status = *payload;

    if(status != 0) {
        AaSysLogPrint(LOGLEVEL_DBG, "query play failed");
        return kGeneralErr;
    }

    void* msg;

    msg = AaSysComCreate(API_MESSAGE_ID_QUIT_RESP, MsgQueue_ControllerBus, MsgQueue_MusicHandler, sizeof(ApiQuitResp));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_QUIT_RESP create failed");
        return kNoMemoryErr;
    }

    ApiQuitResp* pl = AaSysComGetPayload(msg);

    if(status != 0) {
        pl->status = false;
    }
    else {
        pl->status = true;
    }

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_PLAY_RESP send failed");
    }

    return kNoErr;
}

static OSStatus ParseTrackPlay(uint8_t* payload)
{
    u8 type = *(uint8_t*)payload;
    uint8_t status = *(uint8_t*)(payload + sizeof(uint8_t));

    if(status != 0) {
        AaSysLogPrint(LOGLEVEL_DBG, "query play failed");
        return kGeneralErr;
    }

    void* msg;

    msg = AaSysComCreate(API_MESSAGE_ID_PLAY_RESP, MsgQueue_ControllerBus, MsgQueue_MusicHandler, sizeof(ApiPlayResp));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_PLAY_RESP create failed");
        return kNoMemoryErr;
    }

    ApiPlayResp* pl = AaSysComGetPayload(msg);

    pl->type = type;
    if(status != 0) {
        pl->status = false;
    }
    else {
        pl->status = true;
    }

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_PLAY_RESP send failed");
    }

    return kNoErr;
}

static OSStatus ParseTrackNumber(SCBusHeader* header, uint8_t* payload)
{
    if(header->datalen != (sizeof(uint8_t) + sizeof(uint16_t))) {
        AaSysLogPrint(LOGLEVEL_WRN, "data len %d from controller bus incorrect", header->datalen);
        return kGeneralErr;
    }
    
    void* msg;

    msg = AaSysComCreate(API_MESSAGE_ID_TRACKNUM_RESP, MsgQueue_ControllerBus, MsgQueue_MusicHandler, sizeof(ApiTrackNumResp));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNUM_RESP create failed");
        return kNoMemoryErr;
    }

    ApiTrackNumResp* pl = AaSysComGetPayload(msg);
    
    pl->type = *(uint8_t*)payload;
    pl->track_num = *(uint16_t*)(payload + sizeof(uint8_t));

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNUM_RESP send failed");
    }

    AaSysLogPrint(LOGLEVEL_DBG, "send response with type %d tracknum %d at %s", pl->type, pl->track_num, __FILE__);

    return kNoErr;
}

static OSStatus ParseTrackName(SCBusHeader* header, uint8_t* payload)
{
    if(header->datalen <= (2*sizeof(uint8_t) + sizeof(uint16_t))) {
        AaSysLogPrint(LOGLEVEL_WRN, "data len %d from controller bus incorrect", header->datalen);
        return kGeneralErr;
    }

    void* msg;

    msg = AaSysComCreate(API_MESSAGE_ID_TRACKNAME_RESP, MsgQueue_ControllerBus, MsgQueue_MusicHandler, sizeof(ApiTrackNameResp));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNAME_RESP create failed");
        return kNoMemoryErr;
    }

    ApiTrackNameResp* pl = AaSysComGetPayload(msg);
    
    pl->type = *payload;
    pl->status = *(payload + sizeof(uint8_t));
    pl->track_index = *(uint16_t*)(payload + 2*sizeof(uint8_t));

    char* name = (char*)(payload + 2*sizeof(uint8_t) + sizeof(uint16_t));
    sprintf(pl->name, "%s\0", name);

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNUM_RESP send failed");
    }
    
    return kNoErr;
}

static OSStatus ParseVolume(uint8_t* payload)
{
    uint8_t* status = payload;

    void* msg;

    msg = AaSysComCreate(API_MESSAGE_ID_VOLUME_RESP, MsgQueue_ControllerBus, MsgQueue_MusicHandler, sizeof(ApiVolumeResp));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_VOLUME_RESP create failed");
        return kNoMemoryErr;
    }

    ApiVolumeResp* msg_pl = AaSysComGetPayload(msg);
    if(*status == 0) {
        msg_pl->status = true;
    }
    else {
        msg_pl->status = false;
    }

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_VOLUME_RESP send failed");
    }

    return kNoErr;
}

static OSStatus ParseTFCardStatus(uint8_t* payload)
{
    uint8_t* tfstatus = payload;
    uint16_t* tf = (uint16_t*)(payload + sizeof(uint8_t));

    void* msg;

    msg = AaSysComCreate(API_MESSAGE_ID_TFSTATUS_RESP, MsgQueue_ControllerBus, MsgQueue_DeviceHandler, sizeof(ApiTfStatusResp));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TFSTATUS_RESP create failed");
        return kNoMemoryErr;
    }

    ApiTfStatusResp* msg_pl = AaSysComGetPayload(msg);
    if(*tfstatus != 0) {
        msg_pl->status = false;
    }
    else {
        msg_pl->status = true;
    }
    msg_pl->capacity = tf[0];
    msg_pl->free = tf[1];

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TFSTATUS_RESP send failed");
    }

    return kNoErr;
}

bool ControllerBusInit(void)
{
    OSStatus err;

    // f411 reset pin initialize
    MicoGpioInitialize(F411_RESET_PIN, OUTPUT_OPEN_DRAIN_PULL_UP);

    // V2 PCB, spi pin reused for uart, can be remove at V3 PCB
    PinInitForUsart();

    err = user_uartInit();
    if(err != kNoErr) {
        AaSysLogPrint(LOGLEVEL_ERR, "mico uart initialize failed");
        return false;
    }
    
    // start the uart receive thread to handle controller bus data
    err = mico_rtos_create_thread(&bus_recv_handle, MICO_APPLICATION_PRIORITY, "CBusProtoHandler", 
                                ControllerBusProtocolHandler, CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD, 
                                NULL);
    if(err != kNoErr) {
        AaSysLogPrint(LOGLEVEL_ERR, "create controller bus protocol handler thread failed");
        return false;
    }
    else {
        AaSysLogPrint(LOGLEVEL_INF, "create controller bus protocol handler thread success");
    }

    // start the uart send thread to handle request message
    err = mico_rtos_create_thread(&bus_send_handle, MICO_APPLICATION_PRIORITY, "BusRequestSend", 
                                ControllerBusMsgHandler, CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD, 
                                NULL);
    if(err != kNoErr) {
        AaSysLogPrint(LOGLEVEL_ERR, "create controller bus message handler thread failed");
        return false;
    }
    else {
        AaSysLogPrint(LOGLEVEL_INF, "create controller bus message handler thread success");
    }

    AaSysLogPrint(LOGLEVEL_INF, "controller bus initialize success");

    return true;
}


#define CONTROLLERBUS_PIN_MOSI  CBUS_PIN_MOSI
#define CONTROLLERBUS_PIN_MISO  CBUS_PIN_MISO
#define CONTROLLERBUS_PIN_SCK   CBUS_PIN_SCK
#define CONTROLLERBUS_PIN_NSS   CBUS_PIN_NSS

static void PinInitForUsart(void)
{
    MicoGpioInitialize(CONTROLLERBUS_PIN_MISO, INPUT_HIGH_IMPEDANCE);
    MicoGpioInitialize(CONTROLLERBUS_PIN_MOSI, INPUT_HIGH_IMPEDANCE);
    MicoGpioInitialize(CONTROLLERBUS_PIN_SCK, INPUT_HIGH_IMPEDANCE);
    MicoGpioInitialize(CONTROLLERBUS_PIN_NSS, INPUT_HIGH_IMPEDANCE);
}

void ResetF411(void)
{
    MicoGpioOutputLow(F411_RESET_PIN);
    mico_thread_msleep(100);
    MicoGpioOutputHigh(F411_RESET_PIN);
    mico_thread_msleep(100);

    AaSysLogPrint(LOGLEVEL_INF, "f411 rest done");
}

bool CheckTrackType(u8 type)
{
    if(type >= TRACKTYPE_SYSTEM && type <= TRACKTYPE_WECHAT) {
        return true;
    }
    else {
        return false;
    }
}


// end of file


