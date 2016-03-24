


/***

History:
2015-12-22: Ted: Create

*/

#include "mico.h"
#include "json_c/json.h"
#include "SendJson.h"
#include "If_MO.h"
#include "user_debug.h"
#include "ffconf.h"
#include "mp3.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("SendJson", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("SendJson")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("SendJson", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


static bool SendJson(app_context_t *arg, json_object *send_json_object)
{
    bool ret = false;
    const char *upload_data = NULL;

    if(send_json_object == NULL) {
        user_log("[ERR]SendJson: entry with send_json_object NULL");
        return false;
    }
    
    upload_data = json_object_to_json_string(send_json_object);
    if(NULL == upload_data) {
        user_log("[ERR]SendJson: create upload data string error");
    }
    else {
        // upload data string to fogcloud, the seconde param(NULL) means send to defalut topic: '<device_id>/out'
        MiCOFogCloudMsgSend(arg, NULL, (unsigned char*)upload_data, strlen(upload_data));
        user_log("[DBG]SendJson: upload data success \t topic=%s/out \t %s", 
                arg->appConfig->fogcloudConfig.deviceId,
                upload_data);

        ret = true;
    }

    return ret;
}


bool SendJsonInt(app_context_t *arg, char* str, int value)
{
    bool ret = false;
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonInt: create json object error");
    }
    else {
        json_object_object_add(send_json_object, str, json_object_new_int(value)); 
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonDouble(app_context_t *arg, char* str, double value)
{
    bool ret = false;
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonDouble: create json object error");
    }
    else {
        json_object_object_add(send_json_object, str, json_object_new_double(value)); 
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonBool(app_context_t *arg, char* str, bool value)
{
    bool ret;
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonBool: create json object error");
    }
    else {
        json_object_object_add(send_json_object, str, json_object_new_boolean(value)); 
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonLedConf(app_context_t *arg)
{
    bool ret;
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendLedConf: create json object error");
    }
    else {
        json_object_object_add(send_json_object, "LIGHTS-1/LedConfRed", json_object_new_int(GetRedConf()));
        json_object_object_add(send_json_object, "LIGHTS-1/LedConfGreen", json_object_new_int(GetGreenConf())); 
        json_object_object_add(send_json_object, "LIGHTS-1/LedConfBlue", json_object_new_int(GetBlueConf())); 
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonNoDisturbingConf(app_context_t *arg)
{
    bool ret;
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendNoDisturbingConf: create json object error");
    }
    else {
        json_object_object_add(send_json_object, "HEALTH-1/NoDisturbingStartHour", json_object_new_int(GetNoDisturbingStartHour()));
        json_object_object_add(send_json_object, "HEALTH-1/NoDisturbingEndHour", json_object_new_int(GetNoDisturbingEndHour())); 
        json_object_object_add(send_json_object, "HEALTH-1/NoDisturbingStartMinute", json_object_new_int(GetNoDisturbingStartMinute()));
        json_object_object_add(send_json_object, "HEALTH-1/NoDisturbingEndMinute", json_object_new_int(GetNoDisturbingEndMinute())); 
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonPickup(app_context_t *arg)
{
    bool ret;
    u8 index;
    char om_string[64];
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonPickup: create json object error");
    }
    else {
        for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
            sprintf(om_string, "PICKUP-%d/Enable\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_boolean(GetPickUpEnable(index)));
            sprintf(om_string, "PICKUP-%d/SelTrack\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetPickUpSelTrack(index)));
        }
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonPutdown(app_context_t *arg)
{
    bool ret;
    u8 index;
    char om_string[64];
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonPutdown: create json object error");
    }
    else {
        for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
            sprintf(om_string, "PUTDOWN-%d/Enable\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_boolean(GetPutDownEnable(index)));
            sprintf(om_string, "PUTDOWN-%d/RemindDelay\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetPutDownRemindDelay(index)));
            sprintf(om_string, "PUTDOWN-%d/SelTrack\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetPutDownSelTrack(index)));
        }
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonImmediate(app_context_t *arg)
{
    bool ret;
    u8 index;
    char om_string[64];
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonImmediate: create json object error");
    }
    else {
        for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
            sprintf(om_string, "IMMEDIATE-%d/Enable\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_boolean(GetImmediateEnable(index)));
            sprintf(om_string, "IMMEDIATE-%d/SelTrack\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetImmediateSelTrack(index)));
        }
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonSchedule(app_context_t *arg)
{
    bool ret;
    u8 index;
    char om_string[64];
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonSchedule: create json object error");
    }
    else {
        for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
            sprintf(om_string, "SCHEDULE-%d/Enable\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_boolean(GetScheduleEnable(index)));
            sprintf(om_string, "SCHEDULE-%d/RemindHour\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetScheduleRemindHour(index)));
            sprintf(om_string, "SCHEDULE-%d/RemindMinute\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetScheduleRemindMinute(index)));
            sprintf(om_string, "SCHEDULE-%d/RemindTimes\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetScheduleRemindTimes(index)));
            sprintf(om_string, "SCHEDULE-%d/SelTrack\0", index + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(GetScheduleSelTrack(index)));
        }
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonAppointment(app_context_t *arg)
{
    bool ret;
    json_object *send_json_object = NULL;
  
    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonAppointment: create json object error");
    }
    else {
        json_object_object_add(send_json_object, "HEATER-1/AppointmentHour", json_object_new_int(GetAppointmentHour()));
        json_object_object_add(send_json_object, "HEATER-1/AppointmentMinute", json_object_new_int(GetAppointmentMinute())); 
        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}

bool SendJsonTrack(app_context_t *arg)
{
    bool ret;
    json_object *send_json_object = NULL;
    u8 idx;
    u8 music_num;
    char name[_MAX_LFN];
    char om_name[_MAX_LFN];
    char om_string[64];

    send_json_object = json_object_new_object();
    if(NULL == send_json_object){
        user_log("[ERR]SendJsonTrack: create json object error");
    }
    else {
        music_num = MP3_getMp3FileNum("0:/default/");
        user_log("[DBG]SendJsonTrack: path 0:/default/ have mp3 file number %d", music_num);
        music_num = MP3_getMp3FileNum("0:/");
        user_log("[DBG]SendJsonTrack: path 0:/ have mp3 file number %d", music_num);
        
        for(idx=0; idx<music_num; idx++) {
            MP3_getMp3FileName("0:/", idx, (u8*)name);
            
            sprintf(om_string, "TRACK-%d/Index\0", idx + 1);
            json_object_object_add(send_json_object, om_string, json_object_new_int(idx + 1));
            sprintf(om_string, "TRACK-%d/TrackName\0", idx + 1);
            sprintf(om_name, "%s\0", name);
            json_object_object_add(send_json_object, om_string, json_object_new_string(om_name));
        }

        ret = SendJson(arg, send_json_object);
    }

    // free json object memory
    json_object_put(send_json_object);
    send_json_object = NULL;

    return ret;
}


// end of file


