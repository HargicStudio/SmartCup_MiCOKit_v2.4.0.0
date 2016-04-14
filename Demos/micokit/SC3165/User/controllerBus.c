
/***

History:
2016-03-15: Ted: Create

*/

#include "controllerBus.h"
#include "user_debug.h"
#include "TimeUtils.h"
#include "user_uart.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("ControllerBus", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("ControllerBus")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("ControllerBus", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD    0x400


#define CONTROLLERBUS_MAGIC   0x5a
#define CONTROLLERBUS_TAIL    0xaa


bool is_serial_data_print_open = true;

#define CONTROLLERBUS_RECV_BUFFER_LENGTH      USER_UART_BUFFER_LENGTH

uint8_t recv_buffer[CONTROLLERBUS_RECV_BUFFER_LENGTH];

static mico_thread_t bus_recv_ringbuffer_handle = NULL;



static void ControllerBusReceivingHandler(void* arg);
static void PrintSerialData(uint8_t* ptr_buf, uint16_t len);



OSStatus ControllerBusSend(unsigned char *inBuf, unsigned int inBufLen)
{
    user_log("[DBG]ControllerBusSend: send with following data");
    PrintSerialData(inBuf, inBufLen);
    return user_uartSend(inBuf, inBufLen);
}



static void ControllerBusReceivingHandler(void* arg)
{
    // avoid compiling warning
    arg = arg;
    uint16_t received_len;

    user_log("[DBG]ControllerBusReceivingHandler: create controllerbus thread success");

    while(1) {
        received_len = user_uartRecv(recv_buffer, sizeof(SCBusHeader));
        if(received_len == 0) {
            user_log("[ERR]ControllerBusReceivingHandler: do not received any header");
            continue;
        }

        user_log("[DBG]ControllerBusReceivingHandler: receive header length %d", received_len);
        PrintSerialData(recv_buffer, received_len);

        if(received_len != sizeof(SCBusHeader)) {
            user_log("[ERR]ControllerBusReceivingHandler: received header length do not match");
            continue;
        }

        SCBusHeader* header = (SCBusHeader*)recv_buffer;

        // parse the header, get the data length and checksum
        if(header->magic != CONTROLLERBUS_MAGIC || header->tail != CONTROLLERBUS_TAIL) {
            user_log("[ERR]ControllerBusReceivingHandler: magic 0x%02x or tail 0x%02x do not match",
                    header->magic, header->tail);
            continue;
        }
        
        uint16_t datalen = header->datalen;
        uint8_t checksum = header->checksum;

        user_log("[DBG]ControllerBusReceivingHandler: get datalen %d checksum 0x%02x", datalen, checksum);

        // TODO: datalen should not large than (CONTROLLERBUS_RECV_BUFFER_LENGTH - sizeof(SCBusHeader))

        // start to receive data
        uint8_t* payload = recv_buffer + sizeof(SCBusHeader);

        received_len = user_uartRecv(payload, datalen);
        if(received_len == 0) {
            user_log("[ERR]ControllerBusReceivingHandler: do not received any data");
            continue;
        }
        
        user_log("[DBG]ControllerBusReceivingHandler: receive data length %d", received_len);
        PrintSerialData(payload, received_len);

        if(received_len != datalen) {
            user_log("[ERR]ControllerBusReceivingHandler: received data length do not match");
            continue;
        }

#if 0
        // check if data available
        uint8_t data_checksum = 0;
        data_checksum += header->magic;
        data_checksum += header->cmd;
        data_checksum += header->datalen >> 8;
        data_checksum += header->datalen 0x00ff;
        data_checksum += header->tail;
        for(uint16_t idx = 0; idx < datalen; idx++) {
            data_checksum += *(payload + idx);
        }
        if(data_checksum != checksum) {
            user_log("[ERR]ControllerBusReceivingHandler: receive data checksum error, dropping");
            continue;
        }
#endif

        user_log("[DBG]ControllerBusReceivingHandler: parse package complete");
    }

    // normally should not access
//exit:
    user_log("[ERR]ControllerBusReceivingHandler: some fatal error occur, thread dead");
    mico_rtos_delete_thread(NULL);  // delete current thread
}

bool ControllerBusInit(void)
{
    OSStatus err;

    // V2 PCB, spi pin reused for uart, can be remove at V3 PCB
    PinInitForUsart();

    err = user_uartInit();
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: mico uart initialize failed");
        return false;
    }
    
    // start the uart receive thread to handle controller bus data
    err = mico_rtos_create_thread(&bus_recv_ringbuffer_handle, MICO_APPLICATION_PRIORITY, "BusRingBufRecv", 
                                ControllerBusReceivingHandler, CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD, 
                                NULL);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: create controller bus receiving thread failed");
        return false;
    }

    user_log("[DBG]ControllerBusInit: controller bus initialize success");

    return true;
}


#define DATA_NUMBER_PRE_LINE    8

static void PrintSerialData(uint8_t* ptr_buf, uint16_t len)
{
    uint8_t print_idx;

    user_log("[DBG]PrintSerialData: serial data:");
    for(print_idx = 0; print_idx < len; print_idx++) {
        if((print_idx % DATA_NUMBER_PRE_LINE) == 0) {
            printf("\r\ntrack %03d:", print_idx);
        }
        printf(" %02x", *(ptr_buf + print_idx));
    }
    printf("\r\n");
}


#define CONTROLLERBUS_PIN_MOSI  CBUS_PIN_MOSI
#define CONTROLLERBUS_PIN_MISO  CBUS_PIN_MISO
#define CONTROLLERBUS_PIN_SCK   CBUS_PIN_SCK
#define CONTROLLERBUS_PIN_NSS   CBUS_PIN_NSS

void PinInitForUsart(void)
{
    MicoGpioInitialize(CONTROLLERBUS_PIN_MISO, INPUT_HIGH_IMPEDANCE);
    MicoGpioInitialize(CONTROLLERBUS_PIN_MOSI, INPUT_HIGH_IMPEDANCE);
    MicoGpioInitialize(CONTROLLERBUS_PIN_SCK, INPUT_HIGH_IMPEDANCE);
    MicoGpioInitialize(CONTROLLERBUS_PIN_NSS, INPUT_HIGH_IMPEDANCE);
}


// end of file


