


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
#include "controllerBus.h"
#include "AaInclude.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("DeviceMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("DeviceMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("DeviceMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define STACK_SIZE_DEVICE_THREAD    0x400


mico_timer_t timer_device_notify;

static mico_thread_t _device_handler = NULL;

// device notify interval, unit: second
u8 device_notify_interval = 10;//60;


static void DeviceHandler(void* arg);
static void DeviceEstimator(void* arg);
static void PowerNotification(app_context_t *app_context);
static bool PushIntoQueue(float* voltage, uint16_t deepth, uint16_t index, float data);
static float GetQueueAverage(float* voltage, uint16_t deepth);
static void PrintQueueValidValue(float* voltage, uint16_t deepth);
static void SignalStrengthNotification(app_context_t *app_context);
static void TemperatureNotification();
static void QueryTfStatus();
static void HandleTfStatusResponse(void* msg_ptr, app_context_t *app_context);


void DeviceInit(app_context_t *app_context)
{
    OSStatus err;
    
    // start the device monitor thread
    err = mico_rtos_create_thread(&_device_handler, MICO_APPLICATION_PRIORITY, "DeviceHandler", 
                                  DeviceHandler, STACK_SIZE_DEVICE_THREAD, 
                                  app_context );
    if(kNoErr != err) {
        AaSysLogPrint(LOGLEVEL_ERR, "create device thread failed");
    }
    else {
        AaSysLogPrint(LOGLEVEL_INF, "create device thread success");
    }

    // start device timer estimator
    err = mico_init_timer(&timer_device_notify, device_notify_interval*UpTicksPerSecond(), DeviceEstimator, app_context);
    if(kNoErr != err) {
        AaSysLogPrint(LOGLEVEL_ERR, "create timer_device_notify failed");
    }
    else {
        AaSysLogPrint(LOGLEVEL_INF, "create timer_device_notify success");

        err = mico_start_timer(&timer_device_notify);
        if(kNoErr != err) {
            AaSysLogPrint(LOGLEVEL_ERR, "start timer_device_notify failed");
        }
        else {
            AaSysLogPrint(LOGLEVEL_INF, "start timer_device_notify success");
        }
    }
}

static void DeviceHandler(void* arg)
{
    app_context_t *app_context = (app_context_t *)arg;
    void* msg_ptr;
    SMsgHeader* msg;

    AaSysLogPrint(LOGLEVEL_INF, "DeviceHandler thread started");
    
    while(1) {
        msg_ptr = AaSysComReceiveHandler(MsgQueue_DeviceHandler, MICO_WAIT_FOREVER);
        msg = (SMsgHeader*)msg_ptr;

        AaSysLogPrint(LOGLEVEL_DBG, "receive message id 0x%04x", msg->msg_id);

        switch(msg->msg_id) {
            case API_MESSAGE_ID_TFSTATUS_RESP: 
                HandleTfStatusResponse(msg_ptr, app_context);
                break;
            default: 
                AaSysLogPrint(LOGLEVEL_WRN, "no message id 0x%04x", msg->msg_id); 
                break;
        }
        
        if(msg_ptr != NULL) AaSysComDestory(msg_ptr);
    }
}

static void DeviceEstimator(void* arg)
{
    OSStatus err;
    app_context_t *app_context = (app_context_t*)arg;

    err = mico_reload_timer(&timer_device_notify);
    if(err != kNoErr) {
        AaSysLogPrint(LOGLEVEL_ERR, "reload timer_device_notify failed");
    }
    else {
//        user_log("[DBG]DeviceEstimator: reload timer_device_notify success");
    }
    
    PowerNotification(app_context);
    SignalStrengthNotification(app_context);
    QueryTfStatus();
}

#define LOW_POWER_LIMIT         15
#define VOLTAGE_BUFFER_DEEPTH   10

static void PowerNotification(app_context_t *app_context)
{
    bool ret;
    float voltage_tmp = 0;
    static float voltage[VOLTAGE_BUFFER_DEEPTH] = {0};
    static uint16_t vol_buf_idx = 0;
    static uint16_t vol_buf_lenth = 0;

    if(GetBatteryVoltage(&voltage_tmp) != true) {
        AaSysLogPrint(LOGLEVEL_ERR, "get battery voltage_tmp failed");
        return ;
    }

    voltage_tmp *= 100.0;
    
    if(voltage_tmp < BATTERY_VOLTAGE_LOW) {
        AaSysLogPrint(LOGLEVEL_WRN, "battery voltage_tmp below the lowest");
        SetPower(0);
        return ;
    }
    else if(voltage_tmp > BATTERY_VOLTAGE_HIGH) {
        AaSysLogPrint(LOGLEVEL_WRN, "battery voltage_tmp is full");
        SetPower(100);
        return ;
    }
    else {
        if(true != PushIntoQueue(voltage, VOLTAGE_BUFFER_DEEPTH, vol_buf_idx, voltage_tmp)) {
            return ;
        }

        if(vol_buf_idx < VOLTAGE_BUFFER_DEEPTH) {
            vol_buf_idx++;
            if(vol_buf_idx >= VOLTAGE_BUFFER_DEEPTH) {
                vol_buf_idx = 0;
            }
        }
        vol_buf_lenth < VOLTAGE_BUFFER_DEEPTH ? vol_buf_lenth++ : NULL;

        if(vol_buf_lenth < VOLTAGE_BUFFER_DEEPTH) {
            voltage_tmp = GetQueueAverage(voltage, vol_buf_lenth);
        } else {
            voltage_tmp = GetQueueAverage(voltage, VOLTAGE_BUFFER_DEEPTH);
        }

//        PrintQueueValidValue(voltage, VOLTAGE_BUFFER_DEEPTH);

        int percent = (int)((voltage_tmp - BATTERY_VOLTAGE_LOW)*100.0/(BATTERY_VOLTAGE_HIGH - BATTERY_VOLTAGE_LOW));
        SetPower(percent);
//        user_log("[DBG]PowerNotification: current power percent %d", percent);
    }

    SetLowPowerAlarm( GetPower() <= LOW_POWER_LIMIT ? true : false );
//    user_log("[DBG]PowerNotification: current power alarm %s", GetLowPowerAlarm() ? "true" : "false");

    ret = false;
    if(IsPowerChanged()) {
        ret = SendJsonInt(app_context, "DEVICE-1/Power", GetPower());
        AaSysLogPrint(LOGLEVEL_DBG, "Power change to %d with ret %s", GetPower(), ret ? "true" : "false");
    }

    ret = false;
    if(IsLowPowerAlarmChanged()) {
        ret = SendJsonBool(app_context, "DEVICE-1/LowPowerAlarm", GetLowPowerAlarm());
        AaSysLogPrint(LOGLEVEL_DBG, "LowPowerAlarm change to %s with ret %s", 
                GetLowPowerAlarm() ? "true" : "false",
                ret ? "true" : "false");
    }
}

static bool PushIntoQueue(float* voltage, uint16_t deepth, uint16_t index, float data)
{
    if(index >= deepth) {
        AaSysLogPrint(LOGLEVEL_WRN, "error input index %d larger than deepth %d", index, deepth);
        return false;
    }

    voltage[index] = data;
//    user_log("[DBG]PushIntoQueue: voltage[%d] get value %f", index, voltage[index]);

    return true;
}

static float GetQueueAverage(float* voltage, uint16_t deepth)
{
    uint16_t i;
    float ret = voltage[0];

//    user_log("[DBG]GetQueueAverage: get the first value voltage[0] %f", voltage[0]);

    for(i=1; i<deepth; i++) {
        ret = (ret + voltage[i])/2;
//        user_log("[DBG]GetQueueAverage: get the voltage[%d] %f and ret with %f", i, voltage[i], ret);
    }

    return ret;
}

static void PrintQueueValidValue(float* voltage, uint16_t deepth)
{
    uint16_t i;

    for(i=0; i<deepth; i++) {
        AaSysLogPrint(LOGLEVEL_DBG, "voltage[%d] %f", i, voltage[i]);
    }
}

static void SignalStrengthNotification(app_context_t *app_context)
{
    bool ret;
    OSStatus err = kNoErr;
    LinkStatusTypeDef wifi_link_status;
    static int link_strength;
    
    err = micoWlanGetLinkStatus(&wifi_link_status);
    if(kNoErr != err){
      AaSysLogPrint(LOGLEVEL_ERR, "get link status err code(%d)", err);
      return ;
    }

    // will never reach 100 percent, range from 0 to 5
    wifi_link_status.wifi_strength += 15;
    link_strength = wifi_link_status.wifi_strength / 20;

    if(GetSignalStrengh() != link_strength) {
        SetSignalStrengh(link_strength);
    }

    ret = false;
    if(IsSignalStrenghChanged()) {
        ret = SendJsonInt(app_context, "DEVICE-1/SignalStrength", GetSignalStrengh());
        AaSysLogPrint(LOGLEVEL_DBG, "SignalStrength change to %d with ret %s", GetSignalStrengh(), ret ? "true" : "false");
    }
}

static void TemperatureNotification()
{
    // TODO: will support in Release Version 2
    
#if 1
    float temp;

    if(TMP75ReadTemperature(&temp) == true) {
        AaSysLogPrint(LOGLEVEL_DBG, "current temperature %f", temp);
    }
    else {
        AaSysLogPrint(LOGLEVEL_ERR, "get temperature failed");
    }
#endif
}

static void QueryTfStatus()
{
    void* msg;
    
    msg = AaSysComCreate(API_MESSAGE_ID_TFSTATUS_REQ, MsgQueue_DeviceHandler, MsgQueue_ControllerBusSend, sizeof(ApiTfStatusReq));
    if(msg == NULL) {
        AaSysLogPrint(LOGLEVEL_ERR, "ApiQueryTfStatus create failed");
        return ;
    }

    if(kNoErr != AaSysComSend(msg)) {
        AaSysLogPrint(LOGLEVEL_ERR, "ApiQueryTfStatus send failed");
    }
}

static void HandleTfStatusResponse(void* msg_ptr, app_context_t *app_context)
{
    bool ret;
    ApiTfStatusResp* payload = AaSysComGetPayload(msg_ptr);

    AaSysLogPrint(LOGLEVEL_DBG, "get TF status %s capacity %d free %d",
            payload->status ? "true" : "false", payload->capacity, payload->free);

    payload->status ? SetTFStatus(true) : SetTFStatus(false);
    SetTFCapacity(payload->capacity);
    SetTFFree(payload->free);

    ret = false;
    if(IsTFStatusChanged()) {
        ret = SendJsonBool(app_context, "DEVICE-1/TFStatus", GetTFStatus());
        AaSysLogPrint(LOGLEVEL_DBG, "TFStatus change to %s with ret %s", 
                GetTFStatus() ? "true" : "false",
                ret ? "true" : "false");
    }

    ret = false;
    if(IsTFCapacityChanged()) {
        ret = SendJsonDouble(app_context, "DEVICE-1/TFCapacity", GetTFCapacity());
        AaSysLogPrint(LOGLEVEL_DBG, "TFCapacity change to %lf with ret %s", 
                GetTFCapacity(),
                ret ? "true" : "false");
    }

    ret = false;
    if(IsTFFreeChanged()) {
        ret = SendJsonDouble(app_context, "DEVICE-1/TFFree", GetTFFree());
        AaSysLogPrint(LOGLEVEL_DBG, "TFFree change to %lf with ret %s", 
                GetTFFree(),
                ret ? "true" : "false");
    }
}


// end of file


