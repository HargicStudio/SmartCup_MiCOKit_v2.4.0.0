


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

#define MAX_LIGHT   60

#define STACK_SIZE_LIGHTS_THREAD    0x400


static mico_thread_t lights_monitor_thread_handle = NULL;


static void lights_thread(void* arg);


OSStatus LightsInit()
{
    OSStatus err;
    
    // start the lights monitor thread
    err = mico_rtos_create_thread(&lights_monitor_thread_handle, MICO_APPLICATION_PRIORITY, "lights_monitor", 
                                  lights_thread, STACK_SIZE_LIGHTS_THREAD, 
                                  NULL );
    require_noerr_action( err, exit, user_log("[ERR]LightsInit: create lights thread failed") );
    user_log("[DBG]LightsInit: create lights thread success");

exit:
    return err;
}

static void lights_thread(void* arg)
{
    // avoid compiling warning
    arg = arg;
    user_log_trace();

    static u8 led[3];
    static u8 countup_led, countdown_led;

    LED_openRGB(0, 0, 0); //初始化关闭所有灯
    LED_closeRGB();

    led[0] = MAX_LIGHT;
    led[1] = 0;
    led[2] = 0;
    countdown_led = 0;
    countup_led = countdown_led + 1;
  
    /* thread loop */
    while(1){
        
        if(GetEnableNotifyLight()) {
            // TODO: LED cycle breath
            LED_openRGB(led[0], led[1], led[2]);
            user_log("[DBG]lights_thread: R(%d) G(%d) B(%d)", led[0], led[1], led[2]);

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

            mico_thread_msleep(100);
        }
        else {
            if(GetLedSwitch()) {
                LED_openRGB(GetRedConf(), GetGreenConf(), GetBlueConf());
            }
            else {
                LED_openRGB(0, 0, 0);
            }

            mico_thread_sleep(2);
        }
    }
}


// end of file


