


/***

History:
2015-12-20: Ted: Create

*/


#include "DeviceMonitor.h"
#include "mico.h"
#include "If_MO.h"
#include "mp3.h"
#include "TimeUtils.h"
#include "SendJson.h"
#include "user_debug.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("DeviceMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("DeviceMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("DeviceMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


mico_timer_t timer_device_notify;


static void DeviceEstimator(void* arg);
static void MOChangedNotification(app_context_t *app_context);
static void PowerNotification();
static void LowPowerAlarmNotification();
static void SignalStrengthNotification();
static void TemperatureNotification();
static void TFCardNotification();


void DeviceInit(app_context_t *app_context)
{
    OSStatus err;

    err = mico_init_timer(&timer_device_notify, 2*UpTicksPerSecond(), DeviceEstimator, app_context);
    if(kNoErr != err) {
        user_log("[ERR]DeviceInit: create timer_device_notify failed");
    }
    else {
        user_log("[DBG]DeviceInit: create timer_device_notify success");

        err = mico_start_timer(&timer_device_notify);
        if(kNoErr != err) {
            user_log("[ERR]DeviceInit: start timer_device_notify failed");
        }
        else {
            user_log("[DBG]DeviceInit: start timer_device_notify success");
        }
    }
}

static void DeviceEstimator(void* arg)
{
    OSStatus err;
    app_context_t *app_context = (app_context_t*)arg;

    err = mico_reload_timer(&timer_device_notify);
    if(err != kNoErr) {
        user_log("[ERR]DeviceEstimator: reload timer_device_notify failed");
    }
    else {
//        user_log("[DBG]DeviceEstimator: reload timer_device_notify success");
    }
    
    PowerNotification();
    LowPowerAlarmNotification();
    SignalStrengthNotification();
    TFCardNotification();

    MOChangedNotification(app_context);
}

static void MOChangedNotification(app_context_t *app_context)
{
    bool ret;
    do {
        ret = false;
        if(IsPowerChanged()) {
            ret = SendJsonInt(app_context, "DEVICE-1/Power", GetPower());
            user_log("[DBG]MOChangedNotification: Power change to %d", GetPower());
        }
        else if(IsLowPowerAlarmChanged()) {
            ret = SendJsonBool(app_context, "DEVICE-1/LowPowerAlarm", GetLowPowerAlarm());
            user_log("[DBG]MOChangedNotification: LowPowerAlarm change to %s", GetLowPowerAlarm() ? "true" : "false");
        }
        else if(IsSignalStrenghChanged()) {
            ret = SendJsonInt(app_context, "DEVICE-1/SignalStrength", GetSignalStrengh());
            user_log("[DBG]MOChangedNotification: SignalStrength change to %d", GetSignalStrengh());
        }
        else if(IsTemperatureChanged()) {
            ret = SendJsonDouble(app_context, "DEVICE-1/Temperature", GetTemperature());
            user_log("[DBG]MOChangedNotification: Temperature change to %lf", GetTemperature());
        }
        else if(IsTFStatusChanged()) {
            ret = SendJsonBool(app_context, "DEVICE-1/TFStatus", GetTFStatus());
            user_log("[DBG]MOChangedNotification: TFStatus change to %s", GetTFStatus() ? "true" : "false");
        }
        else if(IsTFCapacityChanged()) {
            ret = SendJsonDouble(app_context, "DEVICE-1/TFCapacity", GetTFCapacity());
            user_log("[DBG]MOChangedNotification: TFCapacity change to %lf", GetTFCapacity());
        }
        else if(IsTFFreeChanged()) {
            ret = SendJsonDouble(app_context, "DEVICE-1/TFFree", GetTFFree());
            user_log("[DBG]MOChangedNotification: TFFree change to %lf", GetTFFree());
        }
    }while(ret);
}

static void PowerNotification()
{
    SetPower(100);
}


#define LOW_POWER_LIMIT     15

static void LowPowerAlarmNotification()
{
    SetLowPowerAlarm( GetPower() < LOW_POWER_LIMIT ? true : false );
}

static void SignalStrengthNotification()
{
    OSStatus err = kNoErr;
    LinkStatusTypeDef wifi_link_status;
    
    err = micoWlanGetLinkStatus(&wifi_link_status);
    if(kNoErr != err){
      user_log("[ERR]SignalStrengthNotification: err code(%d)", err);
    }

    if(GetSignalStrengh() != wifi_link_status.wifi_strength) {
        SetSignalStrengh(wifi_link_status.wifi_strength);
    }
}

static void TemperatureNotification()
{
    
}

static void TFCardNotification()
{
    SD_sizeInfo sdSizeInfo;
    
    if(MP3_sdCardDetect() != 0) {
        SetTFStatus(true);
        MP3_getSdSize(&sdSizeInfo);
        
        SetTFCapacity(sdSizeInfo.totalSize);
        SetTFFree(sdSizeInfo.availableSize);
    }
    else {
        SetTFStatus(false);
    }
}


// end of file


