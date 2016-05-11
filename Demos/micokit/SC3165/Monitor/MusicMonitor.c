


/***

History:
2015-12-22: Ted: Create

*/


#include "MusicMonitor.h"
#include "If_MO.h"
#include "user_debug.h"
#include "controllerBus.h"
#include "SendJson.h"
#include "AaInclude.h"
#include <stdio.h>
#include "OMFactory.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("MusicMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("MusicMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("MusicMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define STACK_SIZE_MUSIC_THREAD          0x400


static mico_thread_t music_monitor_thread_handle = NULL;
static u8 _volume_tmp = 0;
static u8 _track_type_index = 0;
static u16 _track_number_index = 0;
static u16 _track_number_all = 0;


static void MusicHandler(void* arg);
static OSStatus HandleVolumeReq(void* msg_ptr);
static OSStatus HandleVolumeResp(void* msg_ptr);
static OSStatus HandleTrackListReq(void* msg_ptr);
static void SendTrackNumberReq(u8 track_type);
static void SendTrackNameReq(u8 t_type, u16 t_idx);
static OSStatus HandleTrackNumResp(void* msg_ptr);
static OSStatus HandleTrackNameResp(void* msg_ptr, app_context_t *app_context);
static OSStatus HandlePlayReq(void* msg_ptr);
static OSStatus HandlePlayResp(void* msg_ptr);
static OSStatus HandleQuitReq(void* msg_ptr);
static OSStatus HandleQuitResp(void* msg_ptr);


OSStatus MusicInit(app_context_t *app_context)
{
    OSStatus err;
    
    // start the music monitor thread
    err = mico_rtos_create_thread(&music_monitor_thread_handle, MICO_APPLICATION_PRIORITY, "MusicHandler", 
                                  MusicHandler, STACK_SIZE_MUSIC_THREAD, 
                                  app_context );
    require_noerr_action( err, exit, AaSysLogPrint(LOGLEVEL_ERR, "create music thread failed") );
    AaSysLogPrint(LOGLEVEL_INF, "create music thread success");

    AaSysLogPrint(LOGLEVEL_INF, "music initialize success");
    
exit:
    return err;
}

static void MusicHandler(void* arg)
{
    app_context_t *app_context = (app_context_t *)arg;
    void* msg_ptr;
    SMsgHeader* msg;

    AaSysLogPrint(LOGLEVEL_INF, "MusicHandler thread started");
    
    while(1) {
        msg_ptr = AaSysComReceiveHandler(MsgQueue_MusicHandler, MICO_WAIT_FOREVER);
        msg = (SMsgHeader*)msg_ptr;

        AaSysLogPrint(LOGLEVEL_DBG, "receive message id 0x%04x", msg->msg_id);

        switch(msg->msg_id) {
            case API_MESSAGE_ID_VOLUME_REQ: 
                HandleVolumeReq(msg_ptr);
                break;
            case API_MESSAGE_ID_VOLUME_RESP:
                HandleVolumeResp(msg_ptr);
                break;
            case API_MESSAGE_ID_TRACKLIST_REQ:
                HandleTrackListReq(msg_ptr);
                break;
            case API_MESSAGE_ID_TRACKNUM_RESP:
                HandleTrackNumResp(msg_ptr);
                if(_track_number_all != 0) {
                    // currently TrackType start from 0 and TrackIndex start from 1
                    _track_number_index++;
                    SendTrackNameReq(_track_type_index, _track_number_index);
                }
                else {
                    // there is no track in this type, query other track type
                    _track_type_index++;
                    if(_track_type_index < TRACKTYPE_MAX) {
                        SendTrackNumberReq(_track_type_index);
                    }
                    // else, all track type have been query
                }
                break;
            case API_MESSAGE_ID_TRACKNAME_RESP:
                HandleTrackNameResp(msg_ptr, app_context);
                _track_number_index++;
                if(_track_number_index <= _track_number_all) {
                    SendTrackNameReq(_track_type_index, _track_number_index);
                }
                else {
                    // query other track type
                    _track_type_index++;
                    if(_track_type_index < TRACKTYPE_MAX) {
                        SendTrackNumberReq(_track_type_index);
                    }
                    // else, all track type have been query
                }
                break;
            case API_MESSAGE_ID_PLAY_REQ:
                HandlePlayReq(msg_ptr);
                break;
            case API_MESSAGE_ID_PLAY_RESP:
                HandlePlayResp(msg_ptr);
                break;
            case API_MESSAGE_ID_QUIT_REQ:
                HandleQuitReq(msg_ptr);
                break;
            case API_MESSAGE_ID_QUIT_RESP:
                HandleQuitResp(msg_ptr);
                break;
            default: 
                AaSysLogPrint(LOGLEVEL_WRN, "no message id 0x%04x", msg->msg_id); 
                break;
        }
        
        if(msg_ptr != NULL) AaSysComDestory(msg_ptr);
    }
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

    AaSysLogPrint(LOGLEVEL_DBG, "get volume %d request", pl->volume);

    void* req_msg;

    // send request to ControllerBus thread    
    req_msg = AaSysComCreate(API_MESSAGE_ID_VOLUME_REQ, MsgQueue_MusicHandler, MsgQueue_ControllerBus, sizeof(ApiVolumeReq));
    if(req_msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_VOLUME_REQ create failed");
        return kGeneralErr;
    }

    ApiVolumeReq* req_pl = AaSysComGetPayload(req_msg);
    req_pl->volume = pl->volume;
    _volume_tmp = pl->volume;

    if(kNoErr != AaSysComSend(req_msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_VOLUME_REQ send failed");
    }

    return kNoErr;
}

static OSStatus HandleVolumeResp(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiVolumeResp)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiVolumeResp) %d", 
                msg->pl_size, sizeof(ApiVolumeResp));
        return kInProgressErr;
    }
    
    ApiVolumeResp* pl = AaSysComGetPayload(msg_ptr);

    AaSysLogPrint(LOGLEVEL_DBG, "get volume response status %s", pl->status ? "true" : "false");

    if(pl->status == true) {
        AaSysLogPrint(LOGLEVEL_DBG, "set volume %d success", _volume_tmp);
        SetVolume(_volume_tmp);
        if(IsVolumeChanged()) {
            OMFactorySave();
        }
    }
    else {
        AaSysLogPrint(LOGLEVEL_WRN, "set volume %d failed", _volume_tmp);

        // TODO: any process
    }

    return kNoErr;
}

static OSStatus HandleTrackListReq(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiTrackListReq)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiTrackListReq) %d", 
                msg->pl_size, sizeof(ApiTrackListReq));
        return kInProgressErr;
    }

    // start to query track number and track name

    // send track number request to ControllerBus thread
    _track_type_index = TRACKTYPE_SYSTEM;
    SendTrackNumberReq(_track_type_index);

    return kNoErr;
}

static void SendTrackNumberReq(u8 track_type)
{
    void* msg;
    
    msg = AaSysComCreate(API_MESSAGE_ID_TRACKNUM_REQ, MsgQueue_MusicHandler, MsgQueue_ControllerBus, sizeof(ApiTrackNumReq));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNUM_REQ create failed");
        return ;
    }

    ApiTrackNumReq* pl = AaSysComGetPayload(msg);
    pl->type = track_type;

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNUM_REQ send failed");
    }
}

static void SendTrackNameReq(u8 t_type, u16 t_idx)
{
    void* msg;
    
    msg = AaSysComCreate(API_MESSAGE_ID_TRACKNAME_REQ, MsgQueue_MusicHandler, MsgQueue_ControllerBus, sizeof(ApiTrackNameReq));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNAME_REQ create failed");
        return ;
    }

    ApiTrackNameReq* pl = AaSysComGetPayload(msg);
    pl->type = t_type;
    pl->track_index = t_idx;

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKNAME_REQ send failed");
    }
}

static OSStatus HandleTrackNumResp(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiTrackNumResp)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiTrackNumResp) %d", 
                msg->pl_size, sizeof(ApiTrackNumResp));
        return kInProgressErr;
    }

    ApiTrackNumResp* pl = AaSysComGetPayload(msg_ptr);

    if(_track_type_index != pl->type) {
        AaSysLogPrint(LOGLEVEL_WRN, "track_number request type %d don't match response type %d", 
                _track_type_index, pl->type);

        return kGeneralErr;
    }

    _track_number_all = pl->track_num;
    _track_number_index = 0;
    
    AaSysLogPrint(LOGLEVEL_DBG, "get track_type %d track_number %d at %s", pl->type, pl->track_num, __FILE__);

    return kNoErr;
}

static OSStatus HandleTrackNameResp(void* msg_ptr, app_context_t *app_context)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiTrackNameResp)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiTrackNameResp) %d", 
                msg->pl_size, sizeof(ApiTrackNameResp));
        return kInProgressErr;
    }

    ApiTrackNameResp* pl = AaSysComGetPayload(msg_ptr);

    if(pl->status != 0) {
        AaSysLogPrint(LOGLEVEL_WRN, "get track_name failed with err %d", pl->status);
        return kGeneralErr;
    }

    if(_track_type_index != pl->type) {
        AaSysLogPrint(LOGLEVEL_WRN, "get track_name type %d don't match response type %d",
                _track_type_index, pl->type);

        return kGeneralErr;
    }

    STrack track;

    track.trackIdx = pl->track_index;
    sprintf(track.trackName, "%s\0", pl->name);

    // upload to cloud
    SendJsonTrack(app_context, pl->type, &track);

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

    AaSysComForward(msg_ptr, MsgQueue_MusicHandler, MsgQueue_ControllerBus);

    return kNoErr;
}

static OSStatus HandlePlayResp(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiPlayResp)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiPlayResp) %d", 
                msg->pl_size, sizeof(ApiPlayResp));
        return kInProgressErr;
    }
    
    ApiPlayResp* pl = AaSysComGetPayload(msg_ptr);

    AaSysLogPrint(LOGLEVEL_DBG, "get play response status %s", pl->status ? "true" : "false");

    if(pl->status == true) {
        AaSysLogPrint(LOGLEVEL_DBG, "track play success");
    }
    else {
        AaSysLogPrint(LOGLEVEL_WRN, "track play failed");

        // TODO: any process
    }

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

    AaSysComForward(msg_ptr, MsgQueue_MusicHandler, MsgQueue_ControllerBus);

    return kNoErr;
}

static OSStatus HandleQuitResp(void* msg_ptr)
{
    SMsgHeader* msg = (SMsgHeader*)msg_ptr;
    if(msg->pl_size != sizeof(ApiQuitResp)) {
        AaSysLogPrint(LOGLEVEL_ERR, "pl_size %d don't match sizeof(ApiQuitResp) %d", 
                msg->pl_size, sizeof(ApiQuitResp));
        return kInProgressErr;
    }
    
    ApiQuitResp* pl = AaSysComGetPayload(msg_ptr);

    AaSysLogPrint(LOGLEVEL_DBG, "get quit response status %s", pl->status ? "true" : "false");

    if(pl->status == true) {
        AaSysLogPrint(LOGLEVEL_DBG, "track quit success");
    }
    else {
        AaSysLogPrint(LOGLEVEL_WRN, "track quit failed");

        // TODO: any process
    }

    return kNoErr;
}


// end of file


