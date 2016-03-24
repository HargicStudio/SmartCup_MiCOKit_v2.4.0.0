


/***

History:
2015-12-20: Ted: Create

*/


#include "DeviceMonitor.h"
#include "mico.h"
#include "If_MO.h"
#include "TimeUtils.h"
#include "SendJson.h"
#include "user_debug.h"
#include "battery.h"
#include "temperature.h"


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
    SignalStrengthNotification();
    TemperatureNotification();
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


#define LOW_POWER_LIMIT     15

static void PowerNotification()
{
    float voltage;

    if(GetBatteryVoltage(&voltage) != true) {
        user_log("[ERR]PowerNotification: get battery voltage failed");
        return ;
    }

    voltage *= 100;
    
    if(voltage < BATTERY_VOLTAGE_LOW) {
        user_log("[WRN]PowerNotification: battery voltage below the lowest");
        SetPower(0);
        return ;
    }
    else if(voltage > BATTERY_VOLTAGE_HIGH) {
        user_log("[WRN]PowerNotification: battery voltage is full");
        SetPower(100);
        return ;
    }
    else {
        int percent = (int)((voltage - BATTERY_VOLTAGE_LOW)*100.0/(BATTERY_VOLTAGE_HIGH - BATTERY_VOLTAGE_LOW));
        SetPower(percent);
        user_log("[DBG]PowerNotification: current power percent %d", percent);
    }

    SetLowPowerAlarm( GetPower() <= LOW_POWER_LIMIT ? true : false );
    user_log("[DBG]PowerNotification: current power alarm %s", GetLowPowerAlarm() ? "true" : "false");
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
    // TODO: will support in Release Version 2
    float temp;

    if(TMP75ReadTemperature(&temp) == true) {
        user_log("[DBG]TemperatureNotification: current temperature %f", temp);
    }
    else {
        user_log("[ERR]TemperatureNotification: get temperature failed");
    }
}

static void TFCardNotification()
{
    // TODO: ask from 411 through spi
    user_log("[DBG]TFCardNotification: get TF card status feature in developing");
}


// end of file


