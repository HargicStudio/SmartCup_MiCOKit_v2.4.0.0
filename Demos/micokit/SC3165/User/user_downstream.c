/**
******************************************************************************
* @file    user_main.c 
* @author  Eshen Wang
* @version V1.0.0
* @date    14-May-2015
* @brief   user main functons in user_main thread.
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/ 

#include "mico.h"
#include "MicoFogCloud.h"
#include "If_MO.h"
#include "SendJson.h"
#include "user_debug.h"
#include "led.h"
#include "OMFactory.h"
#include "MusicMonitor.h"
#include "controllerBus.h"
#include "AaInclude.h"

/* User defined debug log functions
 * Add your own tag like: 'USER_DOWNSTREAM', the tag will be added at the beginning of a log
 * in MICO debug uart, when you call this function.
 */
#ifdef DEBUG
  #define user_log(M, ...) custom_log("USER_DOWNSTREAM", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("USER_DOWNSTREAM")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("USER_DOWNSTREAM", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


extern SDevice gDevice;

static bool ParseOMfromCloud(app_context_t *app_context, const char* string);
bool IsParameterChanged();
static void SendVolumeReq(u8 volume);
static void SendTrackListReq(void);



/* Handle user message from cloud
 * Receive msg from cloud && do hardware operation, like rgbled
 */
void user_downstream_thread(void* arg)
{
  user_log_trace();
  OSStatus err = kUnknownErr;
  app_context_t *app_context = (app_context_t *)arg;
  fogcloud_msg_t *recv_msg = NULL;
      
  require(app_context, exit);
  
  /* thread loop to handle cloud message */
  while(1){
    mico_thread_sleep(1);
        
    // check fogcloud connect status
    if(!app_context->appStatus.fogcloudStatus.isCloudConnected){
      user_log("[ERR]user_downstream_thread: cloud disconnected");
      mico_thread_sleep(2);
      continue;
    }
    
    /* get a msg pointer, points to the memory of a msg: 
     * msg data format: recv_msg->data = <topic><data>
     */
    err = MiCOFogCloudMsgRecv(app_context, &recv_msg, 1000);
    if(kNoErr == err){
      // debug log in MICO dubug uart
      user_log("Cloud => Module: topic[%d]=[%.*s]\tdata[%d]=[%.*s]", 
               recv_msg->topic_len, recv_msg->topic_len, recv_msg->data, 
               recv_msg->data_len, recv_msg->data_len, recv_msg->data + recv_msg->topic_len);
      
      // parse json data from the msg, get led control value
      if(NULL != recv_msg) {
        ParseOMfromCloud(app_context, (const char*)(recv_msg->data + recv_msg->topic_len));
        IsParameterChanged();
      }
      
      // NOTE: must free msg memory after been used.
      if(NULL != recv_msg){
        free(recv_msg);
        recv_msg = NULL;
      }
    }
  }

exit:
  user_log("ERROR: user_downstream_thread exit with err=%d", err);
}

static bool ParseOMfromCloud(app_context_t *app_context, const char* string)
{
    bool ret = false;
    json_object *get_json_object = NULL;
    char om_string[64];
    u8 index;
    
    get_json_object = json_tokener_parse(string);
    if (NULL != get_json_object){
        json_object_object_foreach(get_json_object, key, val) {
            // parse
            if(strcmp(key, "LIGHTS-1/EnableNotifyLight") == 0) {
                SetEnableNotifyLight(json_object_get_boolean(val));
            }
            else if(strcmp(key, "LIGHTS-1/GetEnableNotifyLight") == 0) {
                if(json_object_get_boolean(val) == true) {
                    SendJsonBool(app_context, "LIGHTS-1/EnableNotifyLight", GetEnableNotifyLight());
                }
            }
            else if(strcmp(key, "LIGHTS-1/LedSwitch") == 0) {
                SetLedSwitch(json_object_get_boolean(val));
            }
            else if(strcmp(key, "LIGHTS-1/GetLedSwitch") == 0) {
                if(json_object_get_boolean(val) == true) {
                    SendJsonBool(app_context, "DEVICE-1/LedSwitch", GetLedSwitch());
                }
            }
            else if(strcmp(key, "LIGHTS-1/LedConfRed") == 0) {
                SetRedConf(json_object_get_int(val));
            }
            else if(strcmp(key, "LIGHTS-1/LedConfGreen") == 0) {
                SetGreenConf(json_object_get_int(val));
            }
            else if(strcmp(key, "LIGHTS-1/LedConfBlue") == 0) {
                SetBlueConf(json_object_get_int(val));
            }
            else if(strcmp(key, "LIGHTS-1/GetLedConf") == 0) {
                if(json_object_get_boolean(val) == true) {
                    SendJsonLedConf(app_context);
                }
            }
            else if(strcmp(key, "MUSIC-1/Volume") == 0) {
                u8 volume = json_object_get_int(val);
                AaSysLogPrint(LOGLEVEL_DBG, "get setting volume %d from cloud", volume);
                SendVolumeReq(volume);
            }
            else if(strcmp(key, "MUSIC-1/GetVolume") == 0) {
                if(json_object_get_boolean(val) == true) {
                    SendJsonInt(app_context, "MUSIC-1/Volume", GetVolume());
                }
            }
            else if(strcmp(key, "MUSIC-1/Urlpath") == 0) {
                user_log("[WRN]ParseOMfromCloud: receive %s, download has not support yet", json_object_get_string(val));
                // TODO: will trigger download music through http
            }
            else if(strcmp(key, "MUSIC-1/GetTrackList") == 0) {
                if(json_object_get_boolean(val) == true) {
                    AaSysLogPrint(LOGLEVEL_DBG, "cloud query track list");
                    SendTrackListReq();
                }
            }
            else if(strcmp(key, "MUSIC-1/DelTrack") == 0) {
                user_log("[DBG]ParseOMfromCloud: will delete track(%d) here", json_object_get_int(val));
                // TODO: will delete the track
            }
            else if(strcmp(key, "HEALTH-1/IfNoDisturbing") == 0) {
                SetIfNoDisturbing(json_object_get_boolean(val));
            }
            else if(strcmp(key, "HEALTH-1/NoDisturbingStartHour") == 0) {
                SetNoDisturbingStartHour(json_object_get_int(val));
            }
            else if(strcmp(key, "HEALTH-1/NoDisturbingEndHour") == 0) {
                SetNoDisturbingEndHour(json_object_get_int(val));
            }
            else if(strcmp(key, "HEALTH-1/NoDisturbingStartMinute") == 0) {
                SetNoDisturbingStartMinute(json_object_get_int(val));
            }
            else if(strcmp(key, "HEALTH-1/NoDisturbingEndMinute") == 0) {
                SetNoDisturbingEndMinute(json_object_get_int(val));
            }
            else if(strncmp(key, "HEALTH-1/PICKUP", strlen("HEALTH-1/PICKUP")) == 0) {
                for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
                    sprintf(om_string, "HEALTH-1/PICKUP-%d/Enable\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetPickUpEnable(index, json_object_get_boolean(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/PICKUP-%d/SelTrack\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetPickUpSelTrack(index, json_object_get_int(val));
                        break;
                    }
                }
            }
            else if(strncmp(key, "HEALTH-1/PUTDOWN", strlen("HEALTH-1/PUTDOWN")) == 0) {
                for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
                    sprintf(om_string, "HEALTH-1/PUTDOWN-%d/Enable\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetPutDownEnable(index, json_object_get_boolean(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/PUTDOWN-%d/RemindDelay\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetPutDownRemindDelay(index, json_object_get_int(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/PUTDOWN-%d/SelTrack\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetPutDownSelTrack(index, json_object_get_int(val));
                        break;
                    }
                }
            }
            else if(strncmp(key, "HEALTH-1/IMMEDIATE", strlen("HEALTH-1/IMMEDIATE")) == 0) {
                for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
                    sprintf(om_string, "HEALTH-1/IMMEDIATE-%d/Enable\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetImmediateEnable(index, json_object_get_boolean(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/IMMEDIATE-%d/SelTrack\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetImmediateSelTrack(index, json_object_get_int(val));
                        break;
                    }
                }
            }
            else if(strncmp(key, "HEALTH-1/SCHEDULE", strlen("HEALTH-1/SCHEDULE")) == 0) {
                for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
                    sprintf(om_string, "HEALTH-1/SCHEDULE-%d/Enable\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetScheduleEnable(index, json_object_get_boolean(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/SCHEDULE-%d/RemindHour\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetScheduleRemindHour(index, json_object_get_int(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/SCHEDULE-%d/RemindMinute\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetScheduleRemindMinute(index, json_object_get_int(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/SCHEDULE-%d/RemindTimes\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetScheduleRemindTimes(index, json_object_get_int(val));
                        break;
                    }

                    sprintf(om_string, "HEALTH-1/SCHEDULE-%d/SelTrack\0", index + 1);
                    if(strncmp(key, om_string, strlen(om_string)) == 0) {
                        SetScheduleSelTrack(index, json_object_get_int(val));
                        break;
                    }
                }
            }
            
#if 0
            // for echo debug
            else if(strcmp(key, "DEVICE-1/Power") == 0){
                SetPower(json_object_get_int(val));
            }
            else if(strcmp(key, "DEVICE-1/LowPowerAlarm") == 0) {
                SetLowPowerAlarm(json_object_get_boolean(val));
            }
            else if(strcmp(key, "DEVICE-1/SignalStrength") == 0) {
                SetSignalStrengh(json_object_get_int(val));
            }
            else if(strcmp(key, "DEVICE-1/Temperature") == 0) {
                SetTemperature(json_object_get_double(val));
            }
            else if(strcmp(key, "DEVICE-1/TFStatus") == 0) {
                SetTFStatus(json_object_get_boolean(val));
            }
            else if(strcmp(key, "DEVICE-1/TFCapacity") == 0) {
                SetTFCapacity(json_object_get_double(val));
            }
            else if(strcmp(key, "DEVICE-1/TFFree") == 0) {
                SetTFFree(json_object_get_double(val));
            }
            else if(strcmp(key, "MUSIC-1/TrackNumber") == 0) {
                SendJsonInt(app_context, "MUSIC-1/TrackNumber", json_object_get_int(val));
            }
            else if(strcmp(key, "MUSIC-1/DownLoadRate") == 0) {
                SetDownLoadRate(json_object_get_int(val));
            }
            else if(strcmp(key, "HEALTH-1/DrinkStamp") == 0) {
                SetDrinkStamp(json_object_get_boolean(val));
            }
            else if(strcmp(key, "HEALTH-1/PutDownStamp") == 0) {
                SetPutDownStamp(json_object_get_boolean(val));
            }
#endif
        }

        // free memory of json object
        json_object_put(get_json_object);
        get_json_object = NULL;

        ret = true;
        user_log("[DBG]ParseOMfromCloud: parse json object success");
    }
    else {
        user_log("[ERR]ParseOMfromCloud: parse json object error");
    }

    return ret;
}

bool IsParameterChanged()
{
    bool set_action = false;
    u8 index;

    if(IsEnableNotifyLightChanged()) {
        set_action = true;
        user_log("[DBG]IsParameterChanged: EnableNotifyLight change to %s", GetEnableNotifyLight() ? "true" : "false");
    }
    if(IsLedSwitchChanged()) {
        set_action = true;
        user_log("[DBG]IsParameterChanged: LedSwitch change %s", GetLedSwitch() ? "true" : "false");
    }
    if(IsLedConfChanged()) {
        set_action = true;
        user_log("[DBG]IsParameterChanged: LedConf change R:%d G:%d B:%d", GetRedConf(), GetGreenConf(), GetBlueConf());
    }
    if(IsIfNoDisturbingChanged()) {
        set_action = true;
        user_log("[DBG]IsParameterChanged: IfNoDisturbing change %s", GetIfNoDisturbing() ? "true" : "false");
    }
    if(IsNoDisturbingTimeChanged()) {
        set_action = true;
        user_log("[DBG]IsParameterChanged: NoDisturbingTime change %2d:%2d - %2d:%2d", 
                GetNoDisturbingStartHour(),
                GetNoDisturbingStartMinute(),
                GetNoDisturbingEndHour(),
                GetNoDisturbingEndMinute());
    }
    for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
        if(IsPickupChanged(index)) {
            set_action = true;
            user_log("[DBG]IsParameterChanged: Pickup[%d] change Enable:%s SelTrack:%d", 
                    index, 
                    GetPickUpEnable(index) ? "true" : "false", 
                    GetPickUpSelTrack(index));
        }
    }
    for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
        if(IsPutdownChanged(index)) {
            set_action = true;
            user_log("[DBG]IsParameterChanged: Putdown[%d] change Enable:%s RemindDelay:%d SelTrack:%d",
                    index,
                    GetPutDownEnable(index) ? "true" : "false",
                    GetPutDownRemindDelay(index),
                    GetPutDownSelTrack(index));
        }
    }
    for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
        if(IsImmediateChanged(index)) {
            set_action = true;
            user_log("[DBG]IsParameterChanged: Immediate[%d] change Enable:%s SelTrack:%d",
                    index,
                    GetImmediateEnable(index) ? "true" : "false",
                    GetImmediateSelTrack(index));
        }
    }
    for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
        if(IsScheduleChanged(index)) {
            set_action = true;
            user_log("[DBG]IsParameterChanged: Schedule[%d] change Enable:%s Remind %2d:%2d RemindTimes:%d SelTrack:%d",
                    index,
                    GetScheduleEnable(index) ? "true" : "false",
                    GetScheduleRemindHour(index),
                    GetScheduleRemindMinute(index),
                    GetScheduleRemindTimes(index),
                    GetScheduleSelTrack(index));
        }
    }

    if(set_action) {
        OMFactorySave();
    }

    return set_action;
}

static void SendVolumeReq(u8 volume)
{
    void* msg;
    
    msg = AaSysComCreate(API_MESSAGE_ID_VOLUME_REQ, MsgQueue_DownStream, MsgQueue_MusicHandler, sizeof(ApiVolumeReq));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_VOLUME_REQ create failed");
        return ;
    }

    ApiVolumeReq* pl = AaSysComGetPayload(msg);
    pl->volume = volume;

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_VOLUME_REQ send failed");
    }
}

static void SendTrackListReq(void)
{
    void* msg;
    
    msg = AaSysComCreate(API_MESSAGE_ID_TRACKLIST_REQ, MsgQueue_DownStream, MsgQueue_MusicHandler, sizeof(ApiTrackListReq));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKLIST_REQ create failed");
        return ;
    }

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "API_MESSAGE_ID_TRACKLIST_REQ send failed");
    }
}

// end of file

