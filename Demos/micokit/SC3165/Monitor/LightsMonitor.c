


/***

History:
2015-12-26: Ted: Create

*/


#include "LightsMonitor.h"
#include "If_MO.h"
#include "led.h"
#include "user_debug.h"

#ifdef DEBUG
  #define user_log(M, ...) custom_log("LightsMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("LightsMonitor")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("LightsMonitor", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


enum {
    FLASH_RED,
    FLASH_GREEN,
    FLASH_BLUE
};


#define MAX_LIGHT   80

#define STACK_SIZE_LIGHTS_THREAD    0x400


static mico_thread_t lights_monitor_thread_handle = NULL;


static void lights_thread(void* arg);
static void FlashSecond(uint8_t colour, uint16_t sec);


OSStatus LightsInit(app_context_t *app_context)
{
    OSStatus err;
    
    // start the lights monitor thread
    err = mico_rtos_create_thread(&lights_monitor_thread_handle, MICO_APPLICATION_PRIORITY, "lights_monitor", 
                                  lights_thread, STACK_SIZE_LIGHTS_THREAD, 
                                  app_context );
    require_noerr_action( err, exit, user_log("[ERR]LightsInit: create lights thread failed") );
    user_log("[DBG]LightsInit: create lights thread success");

exit:
    return err;
}

static void lights_thread(void* arg)
{
    app_context_t *app_context = (app_context_t *)arg;
    user_log_trace();

    static u8 led[3];
    static u8 countup_led, countdown_led;

    if(app_context->appStatus.fogcloudStatus.isCloudConnected == true) {
        user_log("[DBG]lights_thread: could is connected");
        FlashSecond(FLASH_BLUE, 3);
    }

    led[0] = MAX_LIGHT;
    led[1] = 0;
    led[2] = 0;
    countdown_led = 0;
    countup_led = countdown_led + 1;
  
    /* thread loop */
    while(1){
        if(app_context->appStatus.fogcloudStatus.isCloudConnected != true) {
            user_log("[WRN]lights_thread: could disconnect");
            FlashSecond(FLASH_RED, 1);
            mico_thread_sleep(2);
            continue;
        }
        
        if(GetEnableNotifyLight()) {
            LedDutyInit((float)led[0], (float)led[1], (float)led[2]);
            LedPwmStart();
//            user_log("[DBG]lights_thread: R(%d) G(%d) B(%d)", led[0], led[1], led[2]);

            // type2
            if(led[countdown_led] == 0 || led[countup_led] >= MAX_LIGHT) {
                countdown_led++;
                if(countdown_led >= 3) {
                    countdown_led = 0;
                }

                countup_led = countdown_led + 1;
                if(countup_led >= 3) {
                    countup_led = 0;
                }
            }
            
            led[countdown_led]--;
            led[countup_led] = MAX_LIGHT - led[countdown_led];

            // stay in this colour for a while
            if(led[countup_led] % 10 == 0) {
                mico_thread_msleep(200);
            }
            else {
                mico_thread_msleep(100);
            }
        }
        else {
            if(GetLedSwitch() == true) {
                LedDutyInit((float)GetRedConf(), (float)GetGreenConf(), (float)GetBlueConf());
                LedPwmStart();
            }
            else {
                LedPwmStop();
            }

            mico_thread_sleep(2);
        }
    }
}

static void FlashSecond(uint8_t colour, uint16_t sec)
{
    uint16_t i;

    for(i=0; i<sec; i++) {
        LedPwmStop();
        user_log("[DBG]FlashSecond: stop led");
        mico_thread_msleep(500);
        switch(colour) {
            case FLASH_RED: LedDutyInit(100.0, 0, 0); break;
            case FLASH_GREEN: LedDutyInit(0, 100.0, 0); break;
            case FLASH_BLUE: LedDutyInit(0, 0, 100.0); break;
        }
        LedPwmStart();
        user_log("[DBG]FlashSecond: startup led flash");
        mico_thread_msleep(500);
    }

    LedPwmStop();
    user_log("[DBG]FlashSecond: finish led flash");
}


// end of file


