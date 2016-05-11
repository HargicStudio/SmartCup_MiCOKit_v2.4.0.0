


/***

History:
2015-12-13: Ted: Create

*/


#include "If_MO.h"
#include "mico.h"
#include "OMFactory.h"
#include "user_debug.h"


#define LEDCONF_MASK                    (0x07)
#define LEDCONF_RED_BIT                 (1<<0)
#define LEDCONF_GREEN_BIT               (1<<1)
#define LEDCONF_BULE_BIT                (1<<2)

#define NODISTURBING_MASK               (0x0f)
#define NODISTURBING_STARTHOUR_BIT      (1<<0)
#define NODISTURBING_ENDHOUR_BIT        (1<<1)
#define NODISTURBING_STARTMINUTE_BIT    (1<<2)
#define NODISTURBING_ENDMINUTE_BIT      (1<<3)

#define PICKUP_MASK                     (0x07)
#define PICKUP_ENABLE_BIT               (1<<0)
#define PICKUP_SELTRACK_BIT             (1<<1)
#define PICKUP_TRACKTYPE_BIT            (1<<2)

#define PUTDOWN_MASK                    (0x0f)
#define PUTDOWN_ENABLE_BIT              (1<<0)
#define PUTDOWN_SELTRACK_BIT            (1<<1)
#define PUTDOWN_REMINDDELAY_BIT         (1<<2)
#define PUTDOWN_TRACKTYPE_BIT           (1<<3)

#define IMMEDIATE_MASK                  (0x07)
#define IMMEDIATE_ENABLE_BIT            (1<<0)
#define IMMEDIATE_SELTRACK_BIT          (1<<1)
#define IMMEDIATE_TRACKTYPE_BIT         (1<<2)

#define SCHEDULE_MASK                   (0x3f)
#define SCHEDULE_ENABLE_BIT             (1<<0)
#define SCHEDULE_SELTRACK_BIT           (1<<1)
#define SCHEDULE_REMINDHOUR_BIT         (1<<2)
#define SCHEDULE_REMINDMINUTE_BIT       (1<<3)
#define SCHEDULE_REMINDTIMES_BIT        (1<<4)
#define SCHEDULE_TRACKTYPE_BIT          (1<<5)


// used for storage buffer
static SLedConf sLedConf_temp;
static SNoDisturbingTime sNoDisturbing_temp;
static SPickup sPickup_temp[MAX_DEPTH_PICKUP];
static SPutdown sPutdown_temp[MAX_DEPTH_PUTDOWN];
static SImmediate sImmediate_temp[MAX_DEPTH_IMMEDIATE];
static SSchedule sSchedule_temp[MAX_DEPTH_SCHEDULE];


SDevice 	gDevice;
SLight  	gLight;
SMusic		gMusic;
STrack  	gTrack;
SHealth 	gHealth;
SPickup 	gPickup[MAX_DEPTH_PICKUP];
SPutdown	gPutdown[MAX_DEPTH_PUTDOWN];
SImmediate	gImmediate[MAX_DEPTH_IMMEDIATE];
SSchedule	gSchedule[MAX_DEPTH_SCHEDULE];

// DEVICE-1
static bool isPowerChanged = false;
static bool isLowPowerAlarmChanged = false;
static bool isSignalStrenghChanged = false;
static bool isTFStatusChanged = false;
static bool isTFCapacityChanged = false;
static bool isTFFreeChanged = false;

// LIGHTS-1
static bool isEnableNotifyLightChanged = false;
static bool isLedSwitchChanged = false;
static u8   isLedConfChanged = 0;

// MUSIC-1
static bool isVolumeChanged = false;
static bool isDownLoadRateChanged = false;

// HEALTH-1
static bool isDrinkPutStatusChanged = false;
static bool isIfNoDisturbingChanged = false;
static u8   isNoDisturbingTimeChanged = 0;
static u8   isPickupChanged[MAX_DEPTH_PICKUP] = {0};
static u8   isPutdownChanged[MAX_DEPTH_PUTDOWN] = {0};
static u8   isImmediateChanged[MAX_DEPTH_IMMEDIATE] = {0};
static u8   isScheduleChanged[MAX_DEPTH_SCHEDULE] = {0};

// HEATER-1
/*
static bool isTemperatureChanged = false;
static bool isTempSwitchChanged = false;
*/


static mico_mutex_t omMutex = NULL;


#ifdef DEBUG
  #define user_log(M, ...) custom_log("If_MO", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("If_MO")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("If_MO", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


void MOInit()
{
    OSStatus err;

    err = mico_rtos_init_mutex(&omMutex);
    if(err) {
        user_log("[ERR]MOInit: create mutex failed");
    }
    else {
        user_log("[DBG]MOInit: create mutex success");
    }

    // init OM, get value from flash
    OMFactoryInit();
}


// *** DEVICE-1 ***

void SetPower(u8 power)
{
    mico_rtos_lock_mutex(&omMutex);

    if(power != gDevice.power) {
        gDevice.power = power;
        isPowerChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetPower()
{
    return gDevice.power;
}

bool IsPowerChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isPowerChanged;
    isPowerChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetLowPowerAlarm(bool flag)
{
    mico_rtos_lock_mutex(&omMutex);

    if(flag != gDevice.lowPowerAlarm) {
        gDevice.lowPowerAlarm = flag;
        isLowPowerAlarmChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetLowPowerAlarm()
{
    return gDevice.lowPowerAlarm;
}

bool IsLowPowerAlarmChanged()
{
    mico_rtos_lock_mutex(&omMutex);

    bool flag = isLowPowerAlarmChanged;
    isLowPowerAlarmChanged = false;
    
    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetSignalStrengh(i16 value)
{
    mico_rtos_lock_mutex(&omMutex);

    if(value != gDevice.signalStrength) {
        gDevice.signalStrength = value;
        isSignalStrenghChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

i16 GetSignalStrengh()
{
    return gDevice.signalStrength;
}

bool IsSignalStrenghChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isSignalStrenghChanged;
    isSignalStrenghChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetTFStatus(bool flag)
{
    mico_rtos_lock_mutex(&omMutex);

    if(flag != gDevice.tfStatus) {
        gDevice.tfStatus = flag;
        isTFStatusChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetTFStatus()
{
    return gDevice.tfStatus;
}

bool IsTFStatusChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isTFStatusChanged;
    isTFStatusChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetTFCapacity(float value)
{
    mico_rtos_lock_mutex(&omMutex);

    if(value != gDevice.tfCapacity) {
        gDevice.tfCapacity = value;
        isTFCapacityChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

float GetTFCapacity()
{
    return gDevice.tfCapacity;
}

bool IsTFCapacityChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isTFCapacityChanged;
    isTFCapacityChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetTFFree(float value)
{
    mico_rtos_lock_mutex(&omMutex);

    if(value != gDevice.tfFree) {
        gDevice.tfFree = value;
        isTFFreeChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

float GetTFFree()
{
    return gDevice.tfFree;
}

bool IsTFFreeChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isTFFreeChanged;
    isTFFreeChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}


// *** LIGHTS-1 ***

void SetEnableNotifyLight(bool flag)
{
    mico_rtos_lock_mutex(&omMutex);

    if(flag != gLight.enableNotifyLight) {
        gLight.enableNotifyLight = flag;
        isEnableNotifyLightChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetEnableNotifyLight()
{
    return gLight.enableNotifyLight;
}

bool IsEnableNotifyLightChanged()
{   
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isEnableNotifyLightChanged;
    isEnableNotifyLightChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetLedSwitch(bool flag)
{
    mico_rtos_lock_mutex(&omMutex);

    if(flag != gLight.ledSwitch) {
        gLight.ledSwitch = flag;
        isLedSwitchChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetLedSwitch()
{
    return gLight.ledSwitch;
}

bool IsLedSwitchChanged()
{   
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isLedSwitchChanged;
    isLedSwitchChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetRedConf(u8 value)
{
    mico_rtos_lock_mutex(&omMutex);

    sLedConf_temp.red = value;
    isLedConfChanged |= LEDCONF_RED_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetRedConf()
{
    return gLight.ledConf.red;
}

void SetGreenConf(u8 value)
{
    mico_rtos_lock_mutex(&omMutex);

    sLedConf_temp.green = value;
    isLedConfChanged |= LEDCONF_GREEN_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetGreenConf()
{
    return gLight.ledConf.green;
}

void SetBlueConf(u8 value)
{
    mico_rtos_lock_mutex(&omMutex);

    sLedConf_temp.blue = value;
    isLedConfChanged |= LEDCONF_BULE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetBlueConf()
{
    return gLight.ledConf.blue;
}

bool IsLedConfChanged()
{
    mico_rtos_lock_mutex(&omMutex);

    bool ret = false;

    if(LEDCONF_MASK == (isLedConfChanged & LEDCONF_MASK)) {
        isLedConfChanged = 0;
        gLight.ledConf = sLedConf_temp;
        ret = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
    
    return ret;
}

// *** MUSIC-1 ***

void SetVolume(u8 value)
{
    mico_rtos_lock_mutex(&omMutex);

    if(value != gMusic.volume) {
        gMusic.volume= value;
        isVolumeChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetVolume()
{
    return gMusic.volume;
}

bool IsVolumeChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isVolumeChanged;
    isVolumeChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetDownLoadRate(u8 value)
{
    mico_rtos_lock_mutex(&omMutex);

    if(value != gMusic.downLoadRate) {
        gMusic.downLoadRate = value;
        isDownLoadRateChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetDownLoadRate()
{
    return gMusic.downLoadRate;
}

bool IsDownLoadRateChanged()
{   
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isDownLoadRateChanged;
    isDownLoadRateChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}


// *** HEALTH-1 ***

void SetDrinkPutStatus(bool flag)
{
    mico_rtos_lock_mutex(&omMutex);

    if(flag != gHealth.drinkPutStatus) {
        gHealth.drinkPutStatus = flag;
        isDrinkPutStatusChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetDrinkPutStatus()
{
    return gHealth.drinkPutStatus;
}

bool IsDrinkPutStatusChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isDrinkPutStatusChanged;
    isDrinkPutStatusChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetIfNoDisturbing(bool flag)
{
    mico_rtos_lock_mutex(&omMutex);

    if(flag != gHealth.ifNoDisturbing) {
        gHealth.ifNoDisturbing = flag;
        isIfNoDisturbingChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetIfNoDisturbing()
{
    return gHealth.ifNoDisturbing;
}

bool IsIfNoDisturbingChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isIfNoDisturbingChanged;
    isIfNoDisturbingChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetNoDisturbingStartHour(u8 starthour)
{
    mico_rtos_lock_mutex(&omMutex);

    sNoDisturbing_temp.startHour = starthour;
    isNoDisturbingTimeChanged |= NODISTURBING_STARTHOUR_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetNoDisturbingStartHour()
{
    return gHealth.noDisturbingTime.startHour;
}

void SetNoDisturbingEndHour(u8 endhour)
{
    mico_rtos_lock_mutex(&omMutex);

    sNoDisturbing_temp.endHour = endhour;
    isNoDisturbingTimeChanged |= NODISTURBING_ENDHOUR_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetNoDisturbingEndHour()
{
    return gHealth.noDisturbingTime.endHour;
}

void SetNoDisturbingStartMinute(u8 startminute)
{
    mico_rtos_lock_mutex(&omMutex);

    sNoDisturbing_temp.startMinute = startminute;
    isNoDisturbingTimeChanged |= NODISTURBING_STARTMINUTE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetNoDisturbingStartMinute()
{
    return gHealth.noDisturbingTime.startMinute;
}

void SetNoDisturbingEndMinute(u8 endminute)
{
    mico_rtos_lock_mutex(&omMutex);

    sNoDisturbing_temp.endMinute = endminute;
    isNoDisturbingTimeChanged |= NODISTURBING_ENDMINUTE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetNoDisturbingEndMinute()
{
    return gHealth.noDisturbingTime.endMinute;
}

bool IsNoDisturbingTimeChanged()
{
    mico_rtos_lock_mutex(&omMutex);

    bool ret = false;
    
    if(NODISTURBING_MASK == (isNoDisturbingTimeChanged & NODISTURBING_MASK)) {
        isNoDisturbingTimeChanged = 0;
        gHealth.noDisturbingTime = sNoDisturbing_temp;
        ret = true;
    }
    
    mico_rtos_unlock_mutex(&omMutex);

    return ret;
}

void SetPickUpEnable(u8 index, bool enable)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_PICKUP) {
        user_log("[ERR]SetPickUpEnable: index(%d) exceed the depth(%d)", index, MAX_DEPTH_PICKUP);
        return ;
    }

    sPickup_temp[index].enable = enable;
    isPickupChanged[index] |= PICKUP_ENABLE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetPickUpEnable(u8 index)
{
    return gPickup[index].enable;
}

void SetPickUpTrackType(u8 index, u8 type)
{
    mico_rtos_lock_mutex(&omMutex);

    if(index >= MAX_DEPTH_PICKUP) {
        user_log("[ERR]SetPickUpTrackType: index(%d) exceed the depth(%d)", index, MAX_DEPTH_PICKUP);
        return ;
    }

    sPickup_temp[index].type = type;
    isPickupChanged[index] |= PICKUP_TRACKTYPE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetPickUpTrackType(u8 index)
{
    return gPickup[index].type;
}

void SetPickUpSelTrack(u8 index, u16 track)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_PICKUP) {
        user_log("[ERR]SetPickUpSelTrack: index(%d) exceed the depth(%d)", index, MAX_DEPTH_PICKUP);
        return ;
    }

    sPickup_temp[index].selTrack = track;
    isPickupChanged[index] |= PICKUP_SELTRACK_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u16 GetPickUpSelTrack(u8 index)
{
    return gPickup[index].selTrack;
}

bool IsPickupChanged(u8 index)
{
    mico_rtos_lock_mutex(&omMutex);

    bool ret = false;
    
    if(PICKUP_MASK == (isPickupChanged[index] & PICKUP_MASK)) {
        isPickupChanged[index] = 0;
        gPickup[index] = sPickup_temp[index];
        ret = true;
    }

    mico_rtos_unlock_mutex(&omMutex);

    return ret;
}

void SetPutDownEnable(u8 index, bool enable)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_PUTDOWN) {
        user_log("[ERR]SetPutDownEnable: index(%d) exceed the depth(%d)", index, MAX_DEPTH_PUTDOWN);
        return ;
    }

    sPutdown_temp[index].enable = enable;
    isPutdownChanged[index] |= PUTDOWN_ENABLE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetPutDownEnable(u8 index)
{
    return gPutdown[index].enable;
}

void SetPutDownTrackType(u8 index, u8 type)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_PUTDOWN) {
        user_log("[ERR]SetPutDownTrackType: index(%d) exceed the depth(%d)", index, MAX_DEPTH_PUTDOWN);
        return ;
    }

    sPutdown_temp[index].type = type;
    isPutdownChanged[index] |= PUTDOWN_TRACKTYPE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetPutDownTrackType(u8 index)
{
    return gPutdown[index].type;
}

void SetPutDownSelTrack(u8 index, u16 track)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_PUTDOWN) {
        user_log("[ERR]SetPutDownSelTrack: index(%d) exceed the depth(%d)", index, MAX_DEPTH_PUTDOWN);
        return ;
    }

    sPutdown_temp[index].selTrack = track;
    isPutdownChanged[index] |= PUTDOWN_SELTRACK_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u16 GetPutDownSelTrack(u8 index)
{
    return gPutdown[index].selTrack;
}

void SetPutDownRemindDelay(u8 index, u16 delay)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_PUTDOWN) {
        user_log("[ERR]SetPutDownRemindDelay: index(%d) exceed the depth(%d)", index, MAX_DEPTH_PUTDOWN);
        return ;
    }

    sPutdown_temp[index].remindDelay = delay;
    isPutdownChanged[index] |= PUTDOWN_REMINDDELAY_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u16 GetPutDownRemindDelay(u8 index)
{
    return gPutdown[index].remindDelay;
}

bool IsPutdownChanged(u8 index)
{
    mico_rtos_lock_mutex(&omMutex);

    bool ret = false;
    
    if(PUTDOWN_MASK == (isPutdownChanged[index] & PUTDOWN_MASK)) {
        isPutdownChanged[index] = 0;
        gPutdown[index] = sPutdown_temp[index];
        ret = true;
    }

    mico_rtos_unlock_mutex(&omMutex);

    return ret;
}

void SetImmediateEnable(u8 index, bool enable)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_IMMEDIATE) {
        user_log("[ERR]SetImmediateEnable: index(%d) exceed the depth(%d)", index, MAX_DEPTH_IMMEDIATE);
        return ;
    }

    sImmediate_temp[index].enable = enable;
    isImmediateChanged[index] |= IMMEDIATE_ENABLE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetImmediateEnable(u8 index)
{
    return gImmediate[index].enable;
}

void SetImmediateTrackType(u8 index, u8 type)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_IMMEDIATE) {
        user_log("[ERR]SetImmediateTrackType: index(%d) exceed the depth(%d)", index, MAX_DEPTH_IMMEDIATE);
        return ;
    }

    sImmediate_temp[index].type = type;
    isImmediateChanged[index] |= IMMEDIATE_TRACKTYPE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetImmediateTrackType(u8 index)
{
    return gImmediate[index].type;
}

void SetImmediateSelTrack(u8 index, u16 track)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_IMMEDIATE) {
        user_log("[ERR]SetImmediateSelTrack: index(%d) exceed the depth(%d)", index, MAX_DEPTH_IMMEDIATE);
        return ;
    }

    sImmediate_temp[index].selTrack= track;
    isImmediateChanged[index] |= IMMEDIATE_SELTRACK_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u16 GetImmediateSelTrack(u8 index)
{
    return gImmediate[index].selTrack;
}

bool IsImmediateChanged(u8 index)
{
    mico_rtos_lock_mutex(&omMutex);

    bool ret = false;
    
    if(IMMEDIATE_MASK == (isImmediateChanged[index] & IMMEDIATE_MASK)) {
        isImmediateChanged[index] = 0;
        gImmediate[index] = sImmediate_temp[index];
        ret = true;
    }

    mico_rtos_unlock_mutex(&omMutex);

    return ret;
}

void SetScheduleEnable(u8 index, bool enable)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_SCHEDULE) {
        user_log("[ERR]SetScheduleEnable: index(%d) exceed the depth(%d)", index, MAX_DEPTH_SCHEDULE);
        return ;
    }

    sSchedule_temp[index].enable = enable;
    isScheduleChanged[index] |= SCHEDULE_ENABLE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetScheduleEnable(u8 index)
{
    return gSchedule[index].enable;
}

void SetScheduleTrackType(u8 index, u8 type)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_SCHEDULE) {
        user_log("[ERR]SetScheduleTrackType: index(%d) exceed the depth(%d)", index, MAX_DEPTH_SCHEDULE);
        return ;
    }

    sSchedule_temp[index].type = type;
    isScheduleChanged[index] |= SCHEDULE_TRACKTYPE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetScheduleTrackType(u8 index)
{
    return gSchedule[index].type;
}


void SetScheduleSelTrack(u8 index, u16 track)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_SCHEDULE) {
        user_log("[ERR]SetScheduleSelTrack: index(%d) exceed the depth(%d)", index, MAX_DEPTH_SCHEDULE);
        return ;
    }

    sSchedule_temp[index].selTrack= track;
    isScheduleChanged[index] |= SCHEDULE_SELTRACK_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u16 GetScheduleSelTrack(u8 index)
{
    return gSchedule[index].selTrack;
}

void SetScheduleRemindHour(u8 index, u8 rh)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_SCHEDULE) {
        user_log("[ERR]SetScheduleRemindHour: index(%d) exceed the depth(%d)", index, MAX_DEPTH_SCHEDULE);
        return ;
    }

    sSchedule_temp[index].remindHour = rh;
    isScheduleChanged[index] |= SCHEDULE_REMINDHOUR_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetScheduleRemindHour(u8 index)
{
    return gSchedule[index].remindHour;
}

void SetScheduleRemindMinute(u8 index, u8 rm)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_SCHEDULE) {
        user_log("[ERR]SetScheduleRemindMinute: index(%d) exceed the depth(%d)", index, MAX_DEPTH_SCHEDULE);
        return ;
    }

    sSchedule_temp[index].remindMinute = rm;
    isScheduleChanged[index] |= SCHEDULE_REMINDMINUTE_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetScheduleRemindMinute(u8 index)
{
    return gSchedule[index].remindMinute;
}

void SetScheduleRemindTimes(u8 index, u8 value)
{
    mico_rtos_lock_mutex(&omMutex);
    
    if(index >= MAX_DEPTH_SCHEDULE) {
        user_log("[ERR]SetScheduleRemindTimes: index(%d) exceed the depth(%d)", index, MAX_DEPTH_SCHEDULE);
        return ;
    }

    sSchedule_temp[index].remindTimes= value;
    isScheduleChanged[index] |= SCHEDULE_REMINDTIMES_BIT;

    mico_rtos_unlock_mutex(&omMutex);
}

u8 GetScheduleRemindTimes(u8 index)
{
    return gSchedule[index].remindTimes;
}

bool IsScheduleChanged(u8 index)
{
    mico_rtos_lock_mutex(&omMutex);

    bool ret = false;
    
    if(SCHEDULE_MASK == (isScheduleChanged[index] & SCHEDULE_MASK)) {
        isScheduleChanged[index] = 0;
        gSchedule[index] = sSchedule_temp[index];
        ret = true;
    }

    mico_rtos_unlock_mutex(&omMutex);

    return ret;
}


// *** HEATER-1 ***
/*
void SetTemperature(float value)
{
    mico_rtos_lock_mutex(&omMutex);

    if(value != gDevice.temperature) {
        gDevice.temperature = value;
        isTemperatureChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

float GetTemperature()
{
    return gDevice.temperature;
}

bool IsTemperatureChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isTemperatureChanged;
    isTemperatureChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}

void SetTempSwitch(bool flag)
{
    mico_rtos_lock_mutex(&omMutex);

    if(flag != gDevice.tempSwitch) {
        gDevice.tempSwitch = flag;
        isTempSwitchChanged = true;
    }

    mico_rtos_unlock_mutex(&omMutex);
}

bool GetTempSwitch()
{
    return gDevice.tempSwitch;
}

bool IsTempSwitchChanged()
{
    mico_rtos_lock_mutex(&omMutex);
    
    bool flag = isTempSwitchChanged;
    isTempSwitchChanged = false;

    mico_rtos_unlock_mutex(&omMutex);
    
    return flag;
}
*/

// end of file


