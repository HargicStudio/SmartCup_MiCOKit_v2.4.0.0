


/***

History:
2016-03-19: Ted: Create

*/

#include "battery.h"
#include "user_debug.h"

#ifdef DEBUG
  #define user_log(M, ...) custom_log("Battery", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("Battery")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("Battery", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define BATTERY_ADC                 MICO_ADC_1
#define BATTERY_ADC_SAMPLE_CYCLE    3


bool BatteryInit(void)
{
  OSStatus err = kUnknownErr;
  
  err = MicoAdcInitialize(BATTERY_ADC, BATTERY_ADC_SAMPLE_CYCLE);
  if(kNoErr != err){
    user_log("[ERR]BatteryInit: MicoAdcInitialize failed");
    return false;
  }

  user_log("[INF]BatteryInit: initialize success");
  
  return true;
}

bool BatteryRead(uint16_t *data)
{
  int ret = 0;
  OSStatus err = kUnknownErr;
  
  // init ADC
  err = MicoAdcInitialize(BATTERY_ADC, BATTERY_ADC_SAMPLE_CYCLE);
  if(kNoErr != err){
    user_log("[ERR]BatteryRead: MicoAdcInitialize failed");
    return false;
  }
  // get ADC data
  err = MicoAdcTakeSample(BATTERY_ADC, data);
  if(kNoErr == err){
    user_log("[DBG]BatteryRead: MicoAdcTakeSample success with sample %d", *data);
    ret = true;   // get data succeed
  }
  else{
    user_log("[ERR]BatteryRead: MicoAdcTakeSample failed");
    ret = false;  // get data error
  }
  
  return ret;
}

bool GetBatteryVoltage(float* voltage)
{
    uint16_t adc_data;
    if(BatteryRead(&adc_data) != true) {
        user_log("[ERR]GetBatteryVoltage: BatteryRead failed");
        return false;
    }

    *voltage = 3.3 * adc_data / (4096 >> 1);
    user_log("[DBG]GetBatteryVoltage: get battery(%f) success", *voltage);

    return true;
}


// end of file


