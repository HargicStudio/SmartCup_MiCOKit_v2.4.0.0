


/***

History:
2016-03-14: Ted: Create

*/


#include "outTrigger.h"
#include "user_debug.h"

#ifdef DEBUG
  #define user_log(M, ...) custom_log("outTrigger", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("outTrigger")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("outTrigger", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define KEY_PIN     OUTERTRG_PIN

#define KEY_CHECK_TIMES     (100/20)


void OuterTriggerInit(mico_gpio_irq_handler_t handler)
{
    MicoGpioInitialize(KEY_PIN, INPUT_HIGH_IMPEDANCE);
    if (handler  != NULL) {
        MicoGpioEnableIRQ( KEY_PIN, IRQ_TRIGGER_BOTH_EDGES, handler, NULL );
    }

    user_log("[INF]OuterTriggerInit: initialize success");
}


EKey GetOuterTriggerStatus(void)
{
    EKey status[2] = {OUTERTRIGGER_NONE, OUTERTRIGGER_NONE};
    u8 cnt = 0;

    while(cnt < KEY_CHECK_TIMES) {
        if(true == MicoGpioInputGet(KEY_PIN)) {
            status[1] = OUTERTRIGGER_PICKUP;
        }
        else {
            status[1] = OUTERTRIGGER_PUTDOWN;
        }

        if(status[1] != status[0]) {
            status[0] = status[1];
            cnt++;
            mico_thread_msleep(20);
        }
        else {
            break;
        }
    }

    if(cnt >= KEY_CHECK_TIMES) {
        user_log("[ERR]GetOuterTriggerStatus: %d status different with each other", cnt);
        return OUTERTRIGGER_ERR;
    }

    return status[1];
}


// end of file


