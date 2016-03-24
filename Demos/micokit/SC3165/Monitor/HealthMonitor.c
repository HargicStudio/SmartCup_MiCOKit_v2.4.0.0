


/***

History:
2015-12-22: Ted: Create

*/


#include "HealthMonitor.h"
#include "mico.h"
#include "If_MO.h"
#include "Sntp.h"
#include "MusicMonitor.h"
#include "key.h"
#include "TimeUtils.h"
#include "SendJson.h"
#include "user_debug.h"


typedef struct SputDownTimer_t {
    u8           index;
    mico_timer_t timer;
} SMngTimer;


#ifdef DEBUG
  #define user_log(M, ...) custom_log("HealthMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("HealthMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("HealthMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define CHECK_CUPSTATUS_TIMEOUT   1000 //ms

#define STACK_SIZE_HEALTH_THREAD    0x400


mico_semaphore_t semaphore_getup;
mico_semaphore_t semaphore_putdown;

static SMngTimer putdown_timer[MAX_DEPTH_PUTDOWN];
static SMngTimer schedule_timer[MAX_DEPTH_SCHEDULE];

static mico_timer_t timer_health_notify;

static mico_thread_t health_monitor_thread_handle = NULL;


extern SPickup 	gPickup[MAX_DEPTH_PICKUP];
extern SPutdown	gPutdown[MAX_DEPTH_PUTDOWN];
extern SImmediate	gImmediate[MAX_DEPTH_IMMEDIATE];
extern SSchedule	gSchedule[MAX_DEPTH_SCHEDULE];

static void health_thread(void* arg);
static bool NoDisturbing();
static bool IsPickupSetting();
static u16 FindPickupTrack();
static bool IsPutDownSetting();
static void startPutDownTimerGroup();
static void PutdownTimeout(void* arg);
static void MOChangedNotification(void *arg);
static void ScheduleTimeout(void* arg);


static void health_thread(void* arg)
{
    // avoid compiling warning
    arg = arg;
    user_log_trace();
    u8 cup_status_temp, cup_status;

    cup_status = cup_status_temp = KEY_getValue();
  
    /* thread loop */
    while(1){
        if(mico_rtos_get_semaphore(&semaphore_getup, CHECK_CUPSTATUS_TIMEOUT)) {
            // key fliter
            cup_status_temp = KEY_getValue();
            if(cup_status != cup_status_temp) {
                cup_status = cup_status_temp;
                
                SetDrinkStamp(true);
                
                if(!NoDisturbing() && IsPickupSetting()) {
                    PlayingSong(FindPickupTrack());
                }
            }
        }

        if(mico_rtos_get_semaphore(&semaphore_putdown, CHECK_CUPSTATUS_TIMEOUT)) {
            // key fliter
            cup_status_temp = KEY_getValue();
            if(cup_status != cup_status_temp) {
                cup_status = cup_status_temp;
                
                SetPutDownStamp(true);

                if(!NoDisturbing() && IsPutDownSetting()) {
                    startPutDownTimerGroup();
                }
            }
        }
    }
}

OSStatus HealthInit(app_context_t *app_context)
{
    u8 idx;
    OSStatus err;
    
    for(idx = 0; idx < MAX_DEPTH_PUTDOWN; idx++) {
        putdown_timer[idx].index = idx;
    }

    //
    for(idx = 0; idx < MAX_DEPTH_SCHEDULE; idx++) {
        schedule_timer[idx].index = idx;
        err = mico_init_timer(&schedule_timer[idx].timer, 60*UpTicksPerSecond(), ScheduleTimeout, &schedule_timer[idx]);
        if(kNoErr != err) {
            user_log("[ERR]HealthInit: create schedule_timer[%d] failed", idx);
        }
        else {
            user_log("[DBG]HealthInit: create schedule_timer[%d] success", idx);
        }

        err = mico_start_timer(&schedule_timer[idx].timer);
        if(kNoErr != err) {
            user_log("[ERR]HealthInit: start schedule_timer[%d] failed", idx);
        }
        else {
            user_log("[DBG]HealthInit: start schedule_timer[%d] success", idx);
        }
    }

    //
    err = mico_rtos_init_semaphore(&semaphore_getup, 1);
    require_noerr_action(err, exit, user_log("[ERR]HealthInit: create semaphore_getup failed"));
    user_log("[DBG]HealthInit: create semaphore_getup success");

    err = mico_rtos_init_semaphore(&semaphore_putdown, 1);
    require_noerr_action(err, exit, user_log("[ERR]HealthInit: create semaphore_putdown failed"));
    user_log("[DBG]HealthInit: create semaphore_putdown success");

    //
    err = mico_init_timer(&timer_health_notify, 2*UpTicksPerSecond(), MOChangedNotification, app_context);
    require_noerr_action(err, exit, user_log("[ERR]HealthInit: create timer_health_notify failed"));
    user_log("[DBG]HealthInit: create timer_health_notify success");

    err = mico_start_timer(&timer_health_notify);
    require_noerr_action(err, exit, user_log("[ERR]HealthInit: start timer_health_notify failed"));
    user_log("[DBG]HealthInit: start timer_health_notify success");

    // start the health monitor thread
    err = mico_rtos_create_thread(&health_monitor_thread_handle, MICO_APPLICATION_PRIORITY, "health_monitor", 
                                  health_thread, STACK_SIZE_HEALTH_THREAD, 
                                  NULL);
    require_noerr_action( err, exit, user_log("[ERR]HealthInit: create health thread failed!"));
    user_log("[DBG]HealthInit: create health thread success!");
    
exit:
    return err;
}

static bool NoDisturbing()
{
    if(!GetIfNoDisturbing()) {
        // do not setting NoDisturbing
        return false;
    }

    struct tm time;
    u16 starting_minute = GetNoDisturbingStartHour()*60 + GetNoDisturbingStartMinute();
    u16 ending_minute = GetNoDisturbingEndHour()*60 + GetNoDisturbingEndMinute();

    sntp_current_time_get(&time);
    
    u16 current_minute = time.tm_hour*60 + time.tm_min;

    if(starting_minute > ending_minute) {
        // the NoDisturbing include 00:00
        if(ending_minute < current_minute && current_minute < starting_minute) {
            // out of the range
            return false;
        }
        else {
            return true;
        }
    }
    else {
        if(starting_minute < current_minute && current_minute < ending_minute) {
            return true;
        }
        else {
            return false;
        }
    }
}

static bool IsPickupSetting()
{
    u8 index;
    
    for(index = 0; index < MAX_DEPTH_PICKUP; index++) {
        if(GetPickUpEnable(index)) {
            return true;
        }
    }

    return false;
}

static u16 FindPickupTrack()
{
    u16 track;
    static u8 count = 0;

    while(1) {
        if(GetPickUpEnable(count)) {
            track = GetPickUpSelTrack(count);
            count = ++count >= MAX_DEPTH_PICKUP ? 0 : count;
            return track;
        }
        else {
            count = ++count >= MAX_DEPTH_PICKUP ? 0 : count;
        }
    }
}

static bool IsPutDownSetting()
{
    u8 index;
    
    for(index = 0; index < MAX_DEPTH_PUTDOWN; index++) {
        if(GetPutDownEnable(index)) {
            return true;
        }
    }

    return false;
}

static void startPutDownTimerGroup()
{
    u8 idx;
    
    for(idx = 0; idx < MAX_DEPTH_PUTDOWN; idx++) {
        if(!GetPutDownEnable(idx)) {
            continue;
        }

        if(GetPutDownRemindDelay(idx) == 0) {
            PlayingSong(GetPutDownSelTrack(idx));
        }
        else {
            // if another putdown action trigger, reset last timers
            if(mico_is_timer_running(&putdown_timer[idx].timer)) {
                mico_stop_timer(&putdown_timer[idx].timer);
                mico_deinit_timer(&putdown_timer[idx].timer);
            }
            
            mico_init_timer(&putdown_timer[idx].timer, 
                            GetPutDownRemindDelay(idx)*60*UpTicksPerSecond(), 
                            PutdownTimeout, 
                            &putdown_timer[idx]);
            mico_start_timer(&putdown_timer[idx].timer);
        }
    }
}

static void PutdownTimeout(void* arg)
{
    SMngTimer* mng = (SMngTimer*)arg;

    mico_deinit_timer(&mng->timer);
    
    // if this putdown tag is disable during timer timeout, do not need to play song
    if(GetPutDownEnable(mng->index)) {
        PlayingSong(GetPutDownSelTrack(mng->index));
    }
}

static void MOChangedNotification(void *arg)
{
    bool ret;
    OSStatus err;
    app_context_t *app_context = (app_context_t *)arg;
    
    err = mico_reload_timer(&timer_health_notify);
    if(err != kNoErr) {
        user_log("[ERR]MOChangedNotification: reload timer_health_notify failed");
    }
    else {
//        user_log("[DBG]MOChangedNotification: reload timer_health_notify success");
    }

    do {
        ret = false;
        if(IsDrinkStampChanged()) {
            ret = SendJsonBool(app_context, "HEALTH-1/DrinkStamp", GetDrinkStamp());
        }
        else if(IsPutDownStampChanged()) {
            ret = SendJsonBool(app_context, "HEALTH-1/PutDownStamp", GetPutDownStamp());
        }
    } while(ret);
}

static void ScheduleTimeout(void* arg)
{
    u8 times;
    OSStatus err;
    struct tm time;
    SMngTimer* mng = (SMngTimer*)arg;

    err = mico_reload_timer(&mng->timer);
    if(err != kNoErr) {
        user_log("[ERR]ScheduleTimeout: reload schedule[%d] failed", mng->index);
    }

    u16 remind_time = GetScheduleRemindHour(mng->index)*60 + GetScheduleRemindMinute(mng->index);
    sntp_current_time_get(&time);
    u16 current_time = time.tm_hour*60 + time.tm_min;

    if(current_time < remind_time) {
        return ;
    }

    times = GetScheduleRemindTimes(mng->index);
    while(times--) {
        PlayingSong(GetScheduleSelTrack(mng->index));
    }
}

// end of file


