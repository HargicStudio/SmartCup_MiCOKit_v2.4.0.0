
/***

History:
2016-03-15: Ted: Create

*/

#include "controllerBus.h"
#include "user_debug.h"
#include "TimeUtils.h"


#ifdef DEBUG
  #define user_log(M, ...) custom_log("ControllerBus", M, ##__VA_ARGS__)
  #define user_log_trace() custom_log_trace("ControllerBus")
#else
#ifdef USER_DEBUG
  #define user_log(M, ...) user_debug("ControllerBus", M, ##__VA_ARGS__)
  #define user_log_trace() (void)0
#endif
#endif


#define SPI_TEST_DEBUG  1


#define CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD    0x400


#define CONTROLLERBUS_IRQ_PIN   CBUS_PIN_TRG


#define CONTROLLERBUS_MAGIC   0x5a
#define CONTROLLERBUS_TAIL    0xaa


const mico_spi_device_t controller_bus_spi =
{
    .port        = MICO_SPI_CBUS,
    .chip_select = CBUS_PIN_NSS,
    .speed       = 40000000,
    .mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH /*| SPI_USE_DMA*/ | SPI_MSB_FIRST),
    .bits        = 8
};

bool is_serial_data_print_open = true;

mico_semaphore_t semaphore_tx = NULL;
mico_semaphore_t semaphore_rx = NULL;
mico_semaphore_t semaphore_resp = NULL;


#define CONTROLLERBUS_QUEUE_LENGTH      1024*2

uint8_t queue_tx[CONTROLLERBUS_QUEUE_LENGTH];
uint8_t queue_rx[CONTROLLERBUS_QUEUE_LENGTH];
uint8_t spi_recv_buffer[CONTROLLERBUS_QUEUE_LENGTH];

ring_buffer_t buf_send;
ring_buffer_t buf_recv;


static mico_thread_t spi_send_ringbuffer_handle = NULL;
static mico_thread_t spi_recv_ringbuffer_handle = NULL;

#ifdef SPI_TEST_DEBUG
    static mico_thread_t spi_test_handle = NULL;
#endif


static void SendingRingBufferHandler(void* arg);
static void ReceivingRingBufferHandler(void* arg);
static void controllerbus_irq_handler();
static void PrintSerialData(uint8_t* ptr_buf, uint16_t len);


static void SendingRingBufferHandler(void* arg)
{
    // avoid compiling warning
    arg = arg;
    uint16_t transfer_size;
    uint8_t* available_data;
    uint32_t bytes_available;

    mico_spi_message_segment_t segment = {NULL, NULL, 0};

    while(1) {
        if(kNoErr != mico_rtos_get_semaphore(&semaphore_tx, MICO_WAIT_FOREVER)) {
            continue;
        }

        user_log("[DBG]SendingRingBufferHandler: get semaphore_tx");

        transfer_size = ring_buffer_used_space(&buf_send);
        user_log("[DBG]SendingRingBufferHandler: get transfer_size %d", transfer_size);
        // there is data in ring buffer need to be sent
        if(transfer_size != 0) {
            do {
                ring_buffer_get_data(&buf_send, &available_data, &bytes_available);
                bytes_available = MIN( bytes_available, transfer_size );

                // send by Mico Spi Interface
                segment.tx_buffer = available_data;
                segment.rx_buffer = NULL;
                segment.length = bytes_available;
                MicoSpiTransfer(&controller_bus_spi, &segment, 1);

                // debug for print sending serial data
                if(is_serial_data_print_open == true) {
                    PrintSerialData(available_data, bytes_available);
                }

                transfer_size -= bytes_available;
                ring_buffer_consume(&buf_send, bytes_available);
            } while(transfer_size != 0);
        }
    }

    // normally should not access
exit:
    user_log("[ERR]SendingRingBufferHandler: some fatal error occur, thread dead");
    mico_rtos_delete_thread(NULL);  // delete current thread
}


static void ReceivingRingBufferHandler(void* arg)
{
    // avoid compiling warning
    arg = arg;
    uint16_t copied_len;

    mico_spi_message_segment_t segment = {NULL, NULL, 0};

    while(1) {
        if(kNoErr != mico_rtos_get_semaphore(&semaphore_rx, MICO_WAIT_FOREVER)) {
            continue;
        }

        user_log("[DBG]ReceivingRingBufferHandler: get semaphore_rx");

        SCBusHeader* header = (SCBusHeader*)spi_recv_buffer;

        // receive by Mico Spi Interface
        segment.tx_buffer = NULL;
        segment.rx_buffer = header;
        // receive header firstly
        segment.length = sizeof(SCBusHeader);
        MicoSpiTransfer(&controller_bus_spi, &segment, 1);

        // parse the header, get the data length and checksum
        uint16_t datalen = header->datalen;
        uint8_t checksum = header->checksum;

        // start to receive data
        uint8_t* payload = (uint8_t*)header + sizeof(SCBusHeader);

        segment.tx_buffer = NULL;
        segment.rx_buffer = payload;
        // receive header firstly
        segment.length = datalen;
        MicoSpiTransfer(&controller_bus_spi, &segment, 1);

        // check if data available
        uint8_t recv_cehcksum = 0;
        for(uint16_t idx = 0; idx < datalen; idx++) {
            recv_cehcksum += *(payload + idx);
        }
        if(recv_cehcksum != checksum) {
            user_log("[ERR]ReceivingRingBufferHandler: receive data checksum error, dropping");
            continue;
        }

        // debug for print receiving serial data
        if(is_serial_data_print_open == true) {
            PrintSerialData(spi_recv_buffer, sizeof(SCBusHeader) + datalen);
        }

        // available data, push into queue
        copied_len = ring_buffer_write(&buf_recv, spi_recv_buffer, sizeof(SCBusHeader) + datalen);
        user_log("[DBG]ReceivingRingBufferHandler: %d length data have been stored into receive queue", copied_len);
        mico_rtos_set_semaphore(&semaphore_resp);
    }

    // normally should not access
exit:
    user_log("[ERR]ReceivingRingBufferHandler: some fatal error occur, thread dead");
    mico_rtos_delete_thread(NULL);  // delete current thread
}

#ifdef SPI_TEST_DEBUG

static void SpiTestDebugThread(void* arg)
{
    // avoid compiling warning
    arg = arg;
    OSStatus err;
    uint16_t copied_len;
    uint8_t test_buf[8];

    while(1) {
        // receive data
        err = mico_rtos_get_semaphore(&semaphore_resp, 2*UpTicksPerSecond());
        if(err == kNoErr) {
            user_log("[DBG]SpiTestDebugThread: data received");

            uint16_t transfer_size = ring_buffer_used_space(&buf_recv);;
            uint8_t* available_data;
            uint32_t bytes_available;

            do {
                ring_buffer_get_data(&buf_recv, &available_data, &bytes_available);
                bytes_available = MIN( bytes_available, transfer_size );

                // debug for print sending serial data
                if(is_serial_data_print_open == true) {
                    PrintSerialData(available_data, bytes_available);
                }

                transfer_size -= bytes_available;
                ring_buffer_consume(&buf_recv, bytes_available);
            } while(transfer_size != 0);
        }

        // sleep
        mico_thread_sleep(2);

        // sending data
        SCBusHeader* header = (SCBusHeader*)test_buf;

        // set sending data
        header->magic = CONTROLLERBUS_MAGIC;
        header->cmd = CONTROLLERBUS_CMD_TFSTATUS;
        header->datalen = 0;
        header->checksum = 0;
        header->tail = CONTROLLERBUS_TAIL;

        // debug for print sending serial data
        if(is_serial_data_print_open == true) {
            
            PrintSerialData(test_buf, sizeof(SCBusHeader));
        }

        // store into sending queue
        copied_len = ring_buffer_write(&buf_send, test_buf, sizeof(SCBusHeader));
        user_log("[DBG]SpiTestDebugThread: %d length data have been stored into sending queue", copied_len);
        // trigger to send
        mico_rtos_set_semaphore(&semaphore_tx);
    }
}

#endif /* end of SPI_TEST_DEBUG */

bool ControllerBusInit(void)
{
    OSStatus err;

    err = MicoSpiInitialize( &controller_bus_spi );
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: mico spi initialize failed");
        return false;
    }

    err = MicoGpioInitialize(CONTROLLERBUS_IRQ_PIN, INPUT_HIGH_IMPEDANCE);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: mico gpio irq initialize failed");
        return false;
    }
    err = MicoGpioEnableIRQ( CONTROLLERBUS_IRQ_PIN, IRQ_TRIGGER_FALLING_EDGE, controllerbus_irq_handler, NULL );
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: mico gpio irq enable failed");
        return false;
    }

    err = mico_rtos_init_semaphore(&semaphore_tx, 4);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: semaphore_tx initialize failed");
        return false;
    }
    err = mico_rtos_init_semaphore(&semaphore_rx, 4);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: semaphore_rx initialize failed");
        return false;
    }
    err = mico_rtos_init_semaphore(&semaphore_resp, 4);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: semaphore_resp initialize failed");
        return false;
    }

    err = ring_buffer_init(&buf_send, queue_tx, CONTROLLERBUS_QUEUE_LENGTH);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: sending ring buffer initialize failed");
        return false;
    }
    err = ring_buffer_init(&buf_recv, queue_rx, CONTROLLERBUS_QUEUE_LENGTH);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: receiving ring buffer initialize failed");
        return false;
    }

    // start the spi ring bugger send/receive thread to handle cotroller bus data
    err = mico_rtos_create_thread(&spi_send_ringbuffer_handle, MICO_APPLICATION_PRIORITY, "BusRingBufSend", 
                                SendingRingBufferHandler, CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD, 
                                NULL);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: create controller bus sending thread failed");
        return false;
    }
    err = mico_rtos_create_thread(&spi_recv_ringbuffer_handle, MICO_APPLICATION_PRIORITY, "BusRingBufRecv", 
                                ReceivingRingBufferHandler, CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD, 
                                NULL);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: create controller bus receiving thread failed");
        return false;
    }

#ifdef SPI_TEST_DEBUG
    err = mico_rtos_create_thread(&spi_test_handle, MICO_APPLICATION_PRIORITY, "spi_test", 
                                SpiTestDebugThread, CONTROLLERBUS_STACK_SIZE_RINGBUFFER_THREAD, 
                                NULL);
    if(err != kNoErr) {
        user_log("[ERR]ControllerBusInit: create controller bus spi test debug thread failed");
        return false;
    }
#endif

    user_log("[DBG]ControllerBusInit: controller bus initialize success");

    return true;
}

static void controllerbus_irq_handler()
{
    if(NULL == semaphore_rx) {
        return ;
    }

    mico_rtos_set_semaphore(&semaphore_rx);
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

void SpiDebugSendTestData(void)
{

}

/*
bool ControllerBusSend(ECBusCmd cmd, u8* data, u16 datalen)
{
    
}
*/

// end of file


