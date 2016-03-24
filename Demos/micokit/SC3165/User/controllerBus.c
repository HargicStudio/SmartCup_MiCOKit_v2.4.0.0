
/***

History:
2016-03-15: Ted: Create

*/

#include "controllerBus.h"
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


#define CONTROLLERBUS_MAGIC   0x5a
#define CONTROLLERBUS_TAIL    0xaa


const mico_spi_device_t controller_bus_spi =
{
    .port        = MICO_SPI_CBUS,
    .chip_select = CBUS_PIN_NSS,
    .speed       = 40000000,
    .mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_USE_DMA | SPI_MSB_FIRST),
    .bits        = 8
};



bool ControllerBusInit(void)
{
    OSStatus err;

    err = MicoSpiInitialize( &controller_bus_spi );
    if(err != kNoErr) {
        return false;
    }

    return true;
}

bool ControllerBusSend(ECBusCmd cmd, u8* data, u16 datalen)
{
    
}

// end of file


