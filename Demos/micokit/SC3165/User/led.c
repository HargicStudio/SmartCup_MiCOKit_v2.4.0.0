


/***

History:
2016-03-15: Ted: Create

*/

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



bool LedDutyInit(float r_duty, float g_duty, float b_duty)
{
    if(r_duty > 100.0 || g_duty > 100.0 || b_duty > 100.0) {
        user_log("[ERR]LedInit: R %f G %f B %f duty cycle error", r_duty, g_duty, b_duty);
        return false;
    }

    MicoPwmInitialize(MICO_PWM_R, 50, r_duty);
    MicoPwmInitialize(MICO_PWM_G, 50, g_duty);
    MicoPwmInitialize(MICO_PWM_B, 50, b_duty);

    user_log("[INF]LedDutyInit: initialize with R %f G %f B %f duty cycle success", r_duty, g_duty, b_duty);

    return true;
}

void LedPwmStart(void)
{
    MicoPwmStart(MICO_PWM_R);
    MicoPwmStart(MICO_PWM_G);
    MicoPwmStart(MICO_PWM_B);
}

void LedPwmStop(void)
{
    MicoPwmStop(MICO_PWM_R);
    MicoPwmStop(MICO_PWM_G);
    MicoPwmStop(MICO_PWM_B);
}



// end of file


