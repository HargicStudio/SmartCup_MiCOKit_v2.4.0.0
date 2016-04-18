


/***

History:
2015-12-17: Ted: Create

*/


#include "OMFactory.h"
#include "If_MO.h"
#include "mico.h"
#include "json_c/json.h"
#include "DeviceMonitor.h"
#include "CheckSumUtils.h"
#include "user_debug.h"
#include "controllerBus.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("OMFactory", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("OMFactory")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("OMFactory", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define OBJECTMODEL_LENGTH_OFFSET   0x0
#define OBJECTMODEL_LENGTH_SIZE     sizeof(uint16_t)

#define OBJECTMODEL_CRC_OFFSET      0x10
#define OBJECTMODEL_CRC_SIZE        sizeof(uint16_t)

#define OBJECTMODEL_PARAMETER_OFFSET 0x20

static void OMFactoryRead();
static bool ParseOM(const char* string);
static void SaveObjectModelDefaultParameter();
static void PrintInitParameter();


void OMFactoryInit()
{
    OMFactoryRead();

    // TODO: get Music track
}

static void OMFactoryRead()
{
    OSStatus err;
    uint16_t objectmodel_length = 0;
    uint16_t objectmodel_crc_from_flash = 0;
    uint16_t objectmodel_crc_by_calc = 0;
    uint32_t para_offset;
    char* conf_string;
    CRC16_Context contex;
    
    para_offset = OBJECTMODEL_LENGTH_OFFSET;
    err = MicoFlashRead(MICO_PARTITION_OBJECTMODEL, &para_offset, (uint8_t *)&objectmodel_length, OBJECTMODEL_LENGTH_SIZE);
    require_noerr_action(err, exit, user_log("[ERR]OMFactoryRead: read flash objectmodel length failed"));
    user_log("[DBG]OMFactoryRead: read flash objectmodel length 0x%x", objectmodel_length);

    if(objectmodel_length == 0xFFFF) {
        user_log("[WRN]OMFactoryRead: there is no objectmodel parameter save in flash");
        SaveObjectModelDefaultParameter();
    }
    else {
        para_offset = OBJECTMODEL_CRC_OFFSET;
        err = MicoFlashRead(MICO_PARTITION_OBJECTMODEL, &para_offset, (uint8_t *)&objectmodel_crc_from_flash, OBJECTMODEL_CRC_SIZE);
        require_noerr_action(err, exit, user_log("[ERR]OMFactoryRead: read flash objectmodel crc failed"));
        user_log("[DBG]OMFactoryRead: read flash objectmodel crc 0x%x", objectmodel_crc_from_flash);

        conf_string = malloc(objectmodel_length);
        if(conf_string == NULL) {
            user_log("[ERR]OMFactoryRead: malloc conf_string failed");
        }
        else {
            para_offset = OBJECTMODEL_PARAMETER_OFFSET;
            err = MicoFlashRead(MICO_PARTITION_OBJECTMODEL, &para_offset, (uint8_t *)conf_string, objectmodel_length);
            require_noerr_action(err, exit, user_log("[ERR]OMFactoryRead: read flash objectmodel parameter failed"));

            CRC16_Init(&contex);
            CRC16_Update( &contex, conf_string, objectmodel_length);
            CRC16_Final( &contex, &objectmodel_crc_by_calc);

            if(objectmodel_crc_by_calc != objectmodel_crc_from_flash) {
                user_log("[ERR]OMFactoryRead: crc from flash(0x%x) != crc by calc(0x%x)", objectmodel_crc_from_flash, objectmodel_crc_by_calc);
            }
            else {
                user_log("[DBG]OMFactoryRead: crc from flash(0x%x) is same with crc by calc", objectmodel_crc_from_flash);
                if(!ParseOM((const char*)conf_string)) {
                    SaveObjectModelDefaultParameter();
                }
            }
        }
    }
    
    PrintInitParameter();

exit:
    if( conf_string!= NULL) free( conf_string );
}

static bool ParseOM(const char* string)
{
    bool ret = false;
    json_object *get_json_object = NULL;
    char om_string[64];
    u8 index;
    unsigned char volume;
    
    get_json_object = json_tokener_parse(string);
    if (NULL != get_json_object){
        json_object_object_foreach(get_json_object, key, val) {
            // parse
            if(strcmp(key, "LIGHTS-1/EnableNotifyLight") == 0) {
                SetEnableNotifyLight(json_object_get_boolean(val));
            }
            else if(strcmp(key, "LIGHTS-1/LedSwitch") == 0) {
                SetLedSwitch(json_object_get_boolean(val));
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
            else if(strcmp(key, "MUSIC-1/Volume") == 0) {
                SetVolume(json_object_get_int(val));
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
        }

        // clear setting flag
        IsEnableNotifyLightChanged();
        IsLedSwitchChanged();
        IsLedConfChanged();
        if(IsVolumeChanged()) {
            user_log("[DBG]ParseOM: volume changed, sending volume config");
            volume = GetVolume();
            ControllerBusSend(CONTROLLERBUS_CMD_VOLUME, &volume, sizeof(volume));
        }
        IsIfNoDisturbingChanged();
        IsNoDisturbingTimeChanged();
        for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
            IsPickupChanged(index);
        }
        for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
            IsPutdownChanged(index);
        }
        for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
            IsImmediateChanged(index);
        }
        for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
            IsScheduleChanged(index);
        }

        // free memory of json object
        json_object_put(get_json_object);
        get_json_object = NULL;

        ret = true;
        user_log("[DBG]ParseOM: parse json object success");
    }
    else {
        user_log("[ERR]ParseOM: parse json object error");
    }

    return ret;
}

void OMFactorySave()
{
    OSStatus err;
    mico_logic_partition_t* objectmodel_partition;
    uint16_t objectmodel_length = 0;
    uint16_t objectmodel_crc = 0;
    uint32_t para_offset;
    json_object *save_json_object = NULL;
    const char *save_data = NULL;
    CRC16_Context contex;
    char om_string[64];
    u8 index;

    save_json_object = json_object_new_object();
    if(NULL == save_json_object){
        user_log("[ERR]OMFactorySave: create json object error");
    }
    else {
        json_object_object_add(save_json_object, "LIGHTS-1/EnableNotifyLight", json_object_new_boolean(GetEnableNotifyLight()));
        json_object_object_add(save_json_object, "LIGHTS-1/LedSwitch", json_object_new_boolean(GetLedSwitch()));
        json_object_object_add(save_json_object, "LIGHTS-1/LedConfRed", json_object_new_int(GetRedConf()));
        json_object_object_add(save_json_object, "LIGHTS-1/LedConfGreen", json_object_new_int(GetGreenConf()));
        json_object_object_add(save_json_object, "LIGHTS-1/LedConfBlue", json_object_new_int(GetBlueConf()));
        json_object_object_add(save_json_object, "MUSIC-1/Volume", json_object_new_int(GetVolume()));
        json_object_object_add(save_json_object, "HEALTH-1/IfNoDisturbing", json_object_new_boolean(GetIfNoDisturbing()));
        json_object_object_add(save_json_object, "HEALTH-1/NoDisturbingStartHour", json_object_new_int(GetNoDisturbingStartHour()));
        json_object_object_add(save_json_object, "HEALTH-1/NoDisturbingStartMinute", json_object_new_int(GetNoDisturbingStartMinute()));
        json_object_object_add(save_json_object, "HEALTH-1/NoDisturbingEndHour", json_object_new_int(GetNoDisturbingEndHour()));
        json_object_object_add(save_json_object, "HEALTH-1/NoDisturbingEndMinute", json_object_new_int(GetNoDisturbingEndMinute()));
        for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
            sprintf(om_string, "HEALTH-1/PICKUP-%d/Enable\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_boolean(GetPickUpEnable(index)));

            sprintf(om_string, "HEALTH-1/PICKUP-%d/SelTrack\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetPickUpSelTrack(index)));
        }
        for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
            sprintf(om_string, "HEALTH-1/PUTDOWN-%d/Enable\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_boolean(GetPutDownEnable(index)));

            sprintf(om_string, "HEALTH-1/PUTDOWN-%d/RemindDelay\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetPutDownRemindDelay(index)));

            sprintf(om_string, "HEALTH-1/PUTDOWN-%d/SelTrack\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetPutDownSelTrack(index)));
        }
        for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
            sprintf(om_string, "HEALTH-1/IMMEDIATE-%d/Enable\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_boolean(GetImmediateEnable(index)));

            sprintf(om_string, "HEALTH-1/IMMEDIATE-%d/SelTrack\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetImmediateSelTrack(index)));
        }
        for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
            sprintf(om_string, "HEALTH-1/SCHEDULE-%d/Enable\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_boolean(GetScheduleEnable(index)));

            sprintf(om_string, "HEALTH-1/SCHEDULE-%d/RemindHour\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetScheduleRemindHour(index)));

            sprintf(om_string, "HEALTH-1/SCHEDULE-%d/RemindMinute\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetScheduleRemindMinute(index)));

            sprintf(om_string, "HEALTH-1/SCHEDULE-%d/RemindTimes\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetScheduleRemindTimes(index)));

            sprintf(om_string, "HEALTH-1/SCHEDULE-%d/SelTrack\0", index + 1);
            json_object_object_add(save_json_object, om_string, json_object_new_int(GetScheduleSelTrack(index)));
        }

        save_data = json_object_to_json_string(save_json_object);
        if(NULL == save_data) {
            user_log("[ERR]OMFactorySave: create data string error");
        }
        else {
            objectmodel_partition = MicoFlashGetInfo( MICO_PARTITION_OBJECTMODEL );
            err = MicoFlashErase( MICO_PARTITION_OBJECTMODEL, 0x0, objectmodel_partition->partition_length );
            require_noerr(err, exit);

            objectmodel_length = strlen(save_data);
            para_offset = OBJECTMODEL_LENGTH_OFFSET;
            err = MicoFlashWrite(MICO_PARTITION_OBJECTMODEL, &para_offset, (uint8_t *)&objectmodel_length, OBJECTMODEL_LENGTH_SIZE);
            require_noerr(err, exit);

            CRC16_Init( &contex );
            CRC16_Update( &contex, save_data, objectmodel_length);
            CRC16_Final( &contex, &objectmodel_crc );
            para_offset = OBJECTMODEL_CRC_OFFSET;
            err = MicoFlashWrite(MICO_PARTITION_OBJECTMODEL, &para_offset, (uint8_t *)&objectmodel_crc, OBJECTMODEL_CRC_SIZE);
            require_noerr(err, exit);

            para_offset = OBJECTMODEL_PARAMETER_OFFSET;
            err = MicoFlashWrite(MICO_PARTITION_OBJECTMODEL, &para_offset, (uint8_t *)save_data, objectmodel_length);
            require_noerr(err, exit);

            user_log("[DBG]OMFactorySave: save data string success");
        }
    }

exit:
    // free json object memory
    json_object_put(save_json_object);
    save_json_object = NULL;
}

static void SaveObjectModelDefaultParameter()
{
    u8 index;
    unsigned char volume;

    user_log("[DBG]SaveObjectModelDefaultParameter: will save default parameter");
    
    SetEnableNotifyLight(false);
    SetLedSwitch(false);
    SetRedConf(0);
    SetGreenConf(50);
    SetBlueConf(0);
    SetVolume(20);
    SetIfNoDisturbing(false);
    SetNoDisturbingStartHour(0);
    SetNoDisturbingStartMinute(0);
    SetNoDisturbingEndHour(0);
    SetNoDisturbingEndMinute(0);
    
    for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
        SetPickUpEnable(index, false);
        SetPickUpSelTrack(index, 0);
    }
    for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
        SetPutDownEnable(index, false);
        SetPutDownRemindDelay(index, 0);
        SetPutDownSelTrack(index, 0);
    }
    for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
        SetImmediateEnable(index, false);
        SetImmediateSelTrack(index, 0);
    }
    for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
        SetScheduleEnable(index, false);
        SetScheduleRemindHour(index, 0);
        SetScheduleRemindMinute(index, 0);
        SetScheduleRemindTimes(index, 0);
        SetScheduleSelTrack(index, 0);
    }

    // clear setting flag
    IsEnableNotifyLightChanged();
    IsLedSwitchChanged();
    IsLedConfChanged();
    if(IsVolumeChanged()) {
        user_log("[DBG]ParseOM: volume changed, sending volume config");
        volume = GetVolume();
        ControllerBusSend(CONTROLLERBUS_CMD_VOLUME, &volume, sizeof(volume));
    }
    IsIfNoDisturbingChanged();
    IsNoDisturbingTimeChanged();
    for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
        IsPickupChanged(index);
    }
    for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
        IsPutdownChanged(index);
    }
    for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
        IsImmediateChanged(index);
    }
    for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
        IsScheduleChanged(index);
    }

    OMFactorySave();
}

static void PrintInitParameter()
{
    u8 index;
    
    user_log("[DBG]PrintInitParameter: LIGHTS-1/EnableNotifyLight: %s", GetEnableNotifyLight() ? "true" : "false");
    user_log("[DBG]PrintInitParameter: LIGHTS-1/LedSwitch: %s", GetLedSwitch() ? "true" : "false");
    user_log("[DBG]PrintInitParameter: LIGHTS-1/LedConfRed: %d", GetRedConf());
    user_log("[DBG]PrintInitParameter: LIGHTS-1/LedConfGreen: %d", GetGreenConf());
    user_log("[DBG]PrintInitParameter: LIGHTS-1/LedConfBlue: %d", GetBlueConf());
    user_log("[DBG]PrintInitParameter: MUSIC-1/Volume: %d", GetVolume());
    user_log("[DBG]PrintInitParameter: HEALTH-1/IfNoDisturbing: %s", GetIfNoDisturbing() ? "true" : "false");
    user_log("[DBG]PrintInitParameter: HEALTH-1/NoDisturbingStartHour: %d", GetNoDisturbingStartHour());
    user_log("[DBG]PrintInitParameter: HEALTH-1/NoDisturbingStartMinute: %d", GetNoDisturbingStartMinute());
    user_log("[DBG]PrintInitParameter: HEALTH-1/NoDisturbingEndHour: %d", GetNoDisturbingEndHour());
    user_log("[DBG]PrintInitParameter: HEALTH-1/NoDisturbingEndMinute: %d", GetNoDisturbingEndMinute());

    for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
        user_log("[DBG]PrintInitParameter: HEALTH-1/PICKUP-%d/Enable: %s", index, GetPickUpEnable(index) ? "true" : "false");
        user_log("[DBG]PrintInitParameter: HEALTH-1/PICKUP-%d/SelTrack: %d", index, GetPickUpSelTrack(index));
    }
    for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
        user_log("[DBG]PrintInitParameter: HEALTH-1/PUTDOWN-%d/Enable: %s", index, GetPutDownEnable(index) ? "true" : "false");
        user_log("[DBG]PrintInitParameter: HEALTH-1/PUTDOWN-%d/RemindDelay: %d", index, GetPutDownRemindDelay(index));
        user_log("[DBG]PrintInitParameter: HEALTH-1/PUTDOWN-%d/SelTrack: %d", index, GetPutDownSelTrack(index));
    }
    for(index = 0; index < MAX_DEPTH_IMMEDIATE; index++) {
        user_log("[DBG]PrintInitParameter: HEALTH-1/IMMEDIATE-%d/Enable: %s", index, GetImmediateEnable(index) ? "true" : "false");
        user_log("[DBG]PrintInitParameter: HEALTH-1/IMMEDIATE-%d/SelTrack: %d", index, GetImmediateSelTrack(index));
    }
    for(index = 0; index < MAX_DEPTH_SCHEDULE; index++) {
        user_log("[DBG]PrintInitParameter: HEALTH-1/SCHEDULE-%d/Enable: %s", index, GetScheduleEnable(index) ? "true" : "false");
        user_log("[DBG]PrintInitParameter: HEALTH-1/SCHEDULE-%d/RemindDelay: %d", index, GetScheduleRemindHour(index));
        user_log("[DBG]PrintInitParameter: HEALTH-1/SCHEDULE-%d/RemindDelay: %d", index, GetScheduleRemindMinute(index));
        user_log("[DBG]PrintInitParameter: HEALTH-1/SCHEDULE-%d/RemindDelay: %d", index, GetScheduleRemindTimes(index));
        user_log("[DBG]PrintInitParameter: HEALTH-1/SCHEDULE-%d/SelTrack: %d", index, GetScheduleSelTrack(index));
    }
}

// end of file


