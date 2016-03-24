


/***

History:
2016-03-17: Ted: Create

*/

#include "led.h"
#include "user_debug.h"

#ifdef DEBUG
  #define user_log(M, ...) custom_log("Temperature", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("Temperature")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("Temperature", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define TMP75_ADDRESS   (0x48 | 0x0)

#define TMP75_POINTERADDR_TEMPREG       0x00
#define TMP75_POINTERADDR_CONFREG       0x01
#define TMP75_POINTERADDR_TLOWREG       0x02
#define TMP75_POINTERADDR_THIGHREG      0x03

#define TMP75_CONFREG_SD                (1<<0)
#define TMP75_CONFREG_TM                (1<<1)
#define TMP75_CONFREG_POL               (1<<2)
#define TMP75_CONFREG_FX                (0x3<<3)
#define TMP75_CONFREG_RX                (0x3<<5)
#define TMP75_CONFREG_OS                (1<<7)


mico_i2c_device_t tmp75_device = 
{
    .port          = MICO_I2C_1,
    .address       = TMP75_ADDRESS,
    .address_width = I2C_ADDRESS_WIDTH_7BIT,
    .speed_mode    = I2C_STANDARD_SPEED_MODE,
};


bool TMP75Init(void)
{
  // I2C init
  MicoI2cFinalize(&tmp75_device);   // in case error
  MicoI2cInitialize(&tmp75_device);

  if( false == MicoI2cProbeDevice(&tmp75_device, 5) ){
    user_log("[ERR]TMP75Init: no i2c device found!");
    return false;
  }
  return true;
}

/*  \Brief: The function is used as I2C bus write
* \Return : Status of the I2C write
* \param dev_addr : The device address of the sensor
* \param reg_addr : Address of the first register, will data is going to be written
* \param reg_data : It is a value hold in the array,
*   will be used for write the value into the register
* \param cnt : The no of byte of data to be write
*/
bool TMP75_IO_Write(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToWrite)
{
  mico_i2c_message_t tmp75_i2c_msg = {NULL, NULL, 0, 0, 0, false};
  OSStatus iError = kUnknownErr;
  uint8_t array[8];
  uint8_t stringpos;
  array[0] = RegisterAddr;
  for (stringpos = 0; stringpos < NumByteToWrite; stringpos++) {
    array[stringpos + 1] = *(pBuffer + stringpos);
  }
  
  iError = MicoI2cBuildTxMessage(&tmp75_i2c_msg, array, NumByteToWrite + 1, 3);
  if(kNoErr != iError){
    user_log("[ERR]TMP75_IO_Write: MicoI2cBuildTxMessage failed");
    return false; 
  }
  iError = MicoI2cTransfer(&tmp75_device, &tmp75_i2c_msg, 1);
  if(kNoErr != iError){
    user_log("[ERR]TMP75_IO_Write: MicoI2cTransfer failed");
    return false;
  }
  
  return true;
}

/*  \Brief: The function is used as I2C bus read
* \Return : Status of the I2C read
* \param dev_addr : The device address of the sensor
* \param reg_addr : Address of the first register, will data is going to be read
* \param reg_data : This data read from the sensor, which is hold in an array
* \param cnt : The no of byte of data to be read
*/
bool TMP75_IO_Read(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToRead)
{
  mico_i2c_message_t tmp75_i2c_msg = {NULL, NULL, 0, 0, 0, false};
  OSStatus iError = kUnknownErr;
  uint8_t array[8] = {0};
  array[0] = RegisterAddr;
  
  iError = MicoI2cBuildCombinedMessage(&tmp75_i2c_msg, array, pBuffer, 1, NumByteToRead, 3);
  if(kNoErr != iError){
    user_log("[ERR]TMP75_IO_Read: MicoI2cBuildCombinedMessage failed");
    return false; 
  }
  iError = MicoI2cTransfer(&tmp75_device, &tmp75_i2c_msg, 1);
  if(kNoErr != iError){
    user_log("[ERR]TMP75_IO_Read: MicoI2cTransfer failed");
    return false;
  }
  return true;
}

bool TemperatureInit(void) 
{
    u8 confReg;
    u8 confRegAgain;

    if(TMP75Init() != true) {
        user_log("[ERR]TemperatureInit: TMP75Init failed");
        return false;
    }

    if(TMP75_IO_Read(&confReg, TMP75_POINTERADDR_CONFREG, 1) != true) {
        user_log("[ERR]TemperatureInit: TMP75_IO_Read CONFREG failed");
        return false;
    }

    // set resolution as 12bits(0.0625 C)
//    confReg &= ~TMP75_CONFREG_RX;
    confReg |= TMP75_CONFREG_RX;
    if(TMP75_IO_Write(&confReg, TMP75_POINTERADDR_CONFREG, 1) != true) {
        user_log("[ERR]TemperatureInit: TMP75_IO_Write CONFREG failed");
        return false;
    }

    if(TMP75_IO_Read(&confRegAgain, TMP75_POINTERADDR_CONFREG, 1) != true) {
        user_log("[ERR]TemperatureInit: TMP75_IO_Read CONFREG again failed");
        return false;
    }

    if(confRegAgain != confReg) {
        user_log("[ERR]TemperatureInit: set CONFREG failed");
        return false;
    }

    user_log("[INF]TemperatureInit: set CONFREG(0x%02x) success", confRegAgain);

    return true;
}

bool TMP75ReadTemperature(float* temperature)
{
    u8 temp[2];
    bool ret = false;

    ret = TMP75_IO_Read(temp, TMP75_POINTERADDR_TEMPREG, 2);
    if(ret != true) {
        user_log("[ERR]TMP75ReadTemperature: TMP75_IO_Read failed");
        return ret;
    }

    user_log("[DBG]TMP75ReadTemperature: get temperature reg byte1(0x%02x) byte2(0x%02x) successful", temp[0], temp[1]);

    *temperature = 0;
    *temperature += temp[0];
    *temperature += (temp[1] >> 4)*0.0625;

    return ret;
}


// end of file


