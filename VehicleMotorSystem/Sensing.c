/* std Header files */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>
#include <xdc/cfg/global.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/gates/GateHwi.h>
/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART.h>
/* Board Header file */
#include "Board.h"
/* Graphics header files */
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "Drivers/Kentec320x240x16_ssd2119_spi.h"
#include "Drivers/touch.h"
/* I2C driver header files */
#include "Drivers/opt3001.h"
#include "Drivers/i2cOptDriver.h"
/* Definition header file */
#include "Drivers/globaldefines.h"
#include "Drivers/motorlib.h"
#include "Drivers/tmp107.h"
/* Other useful library files */
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
//#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "Drivers/opt3001.h"

#define OPT3001_I2C_ADDRESS             0x47

/* Register addresses */
#define REG_RESULT                      0x00
#define REG_CONFIG                      0x01
#define REG_LOW_LIMIT                   0x02
#define REG_HIGH_LIMIT                  0x03

I2C_Handle i2c;

volatile int lux = 0;
volatile int luxArray[] = {0,0,0,0,0,0,0,0,0,0,
                           0,0,0,0,0,0,0,0,0,0,
                           0,0,0,0,0,0,0,0,0,0};

void readOPT3001(){
    UChar readBuffer[2];
    I2C_Transaction i2cTransactionRead_OPT;
    UChar writeBuffer[3];
    Bool transferOK;

    i2cTransactionRead_OPT.slaveAddress = OPT3001_I2C_ADDRESS; /* 7-bit peripheral slave address */
    i2cTransactionRead_OPT.writeBuf = writeBuffer; /* Buffer to be written */
    i2cTransactionRead_OPT.writeCount = 1; /* Number of bytes to be written */
    i2cTransactionRead_OPT.readBuf = readBuffer; /* Buffer to be read */
    i2cTransactionRead_OPT.readCount = 2; /* Number of bytes to be read */

    uint16_t  rawData = 0;
    float     convertedLux = 0;
    while(1){
        writeBuffer[0] = REG_RESULT;
        transferOK = I2C_transfer(i2c, &i2cTransactionRead_OPT); /* Perform I2C transfer */
        rawData = (readBuffer[0] << 8) | readBuffer[1];

        if (transferOK) {
            sensorOpt3001Convert(rawData, &convertedLux);
            lux = (int) convertedLux;
            int i;
            for (i = 0; i < 29; i++) { luxArray[i] = luxArray[i+1]; }
            luxArray[29] = lux;
        }
        Task_sleep(300);
    }
}

void setupI2C(){
    // I2C Parameters
    UInt peripheralNum = 0; /* Such as I2C0 */
    I2C_Params i2cParams;
    I2C_Transaction i2cTransactionWrite;
    UChar writeBuffer[3];
    Bool transferOK;

    i2cTransactionWrite.slaveAddress = OPT3001_I2C_ADDRESS; /* 7-bit peripheral slave address */
    i2cTransactionWrite.writeBuf = writeBuffer; /* Buffer to be written */
    i2cTransactionWrite.writeCount = 3; /* Number of bytes to be written */
    i2cTransactionWrite.readBuf = NULL; /* Buffer to be read */
    i2cTransactionWrite.readCount = 0; /* Number of bytes to be read */

    // Open I2C connection
    I2C_Params_init(&i2cParams);
    i2cParams.transferMode = I2C_MODE_BLOCKING;
    i2cParams.transferCallbackFxn = NULL;
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(peripheralNum, &i2cParams);
    if (i2c == NULL) {
        System_printf("i2c failed\n");
        System_flush();
    }
    System_printf("I2C Initialized\n");
    System_flush();

    // Init OPT3001
    writeBuffer[0] = REG_CONFIG;
    writeBuffer[1] = 0xC0;
    writeBuffer[2] = 0x10;
    transferOK = I2C_transfer(i2c, &i2cTransactionWrite); /* Perform I2C transfer */

    writeBuffer[1] = 0xC4;
    writeBuffer[2] = 0x10;
    transferOK = I2C_transfer(i2c, &i2cTransactionWrite); /* Perform I2C transfer */

    System_printf("OPT3001 Initialized\n");
    System_flush();
}




void readTMP107() {

}
//
//bool setupTMP107UART() {
//    UART_Params uartParams;
//
//    /* Create a UART with data processing off. */
//    UART_Params_init(&uartParams);
//    uartParams.readMode = UART_MODE_BLOCKING;
//    uartParams.readReturnMode = UART_RETURN_FULL;
//    uartParams.readTimeout = TMP107_Timeout*2;
//    uartParams.readEcho = UART_ECHO_OFF;
//    uartParams.baudRate       = 115200;
//
//    uart = UART_open(Board_UART7, &uartParams);
//
//    if (uart == NULL) {
//        return false;
//    }
//    return true;
//}
//
//bool TMP107Transmit(UInt8 tx_data[], int tx_size) {
//    int idx = 0;
//    UInt8 echo;
//    for (idx = 0; idx < tx_size; idx++) {
//        UART_write(uart, &tx_data[idx], 1);
//        UART_read(uart, &echo, 1);
//        if (tx_data[idx] != echo) {
//            return false;
//        }
//    }
//    return true;
//}
//
//void TMP107Receive(UInt8 rx_data[], int rx_size) {
//    int idx = 0;
//    UInt8 echo;
//    for (idx = 0; idx < rx_size; idx++) {
//        UART_read(uart, &echo, 1);
//    }
//}

UART_Handle TMP107InitUart() {
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.readMode = UART_MODE_BLOCKING;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readTimeout = TMP107_Timeout*2;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 115200;
    UART_Handle uartTemp = UART_open(Board_UART7, &uartParams);

    if (uartTemp == NULL) {
        System_printf("Error opening the motor temp UART");
        System_flush();
    }
    return uartTemp;
}

void TMP107WaitForEcho(UART_Handle uart, char tx_size, char* rx_data, char rx_size){
    // Used after a call to Transmit, this function will exit once the transmit echo
    // and any additional rx bytes are received. it will also exit due to time out
    // if timeout_ms lapses without receiving new bytes.
    // Echo of cal byte + echo of transmission + additional bytes if read command
    char expected_rxcnt = tx_size + rx_size + 1; // +1 as stop bit seems to be echoed too
    char tmp107_rx[32]; // receive buffer: filled by UART

    UART_read(uart, &tmp107_rx, expected_rxcnt);

    // Copy bytes received from UART buffer to user supplied array
    char i;
    if (rx_size > 0) {
        for (i = 0; i < rx_size; i++) {
            rx_data[i] = tmp107_rx[tx_size + i];
        }
    }
}

void TMP107Transmit(UART_Handle uart, char* tx_data, char tx_size){
    UART_write(uart, tx_data, tx_size);
}

char TMP107Init(UART_Handle uart) {
    // Initialise the chain and return the last device address
    char tx_size = 3;
    char rx_size = 1;
    char tx[3];
    char rx[1];

    tx[0] = 0x55; // Calibration Byte
    tx[1] = 0x95; // AddressInit command code
    // Give the first device logical address 1
    tx[2] = 0x5 | TMP107_Encode5bitAddress(0x1);
    TMP107Transmit(uart, tx, tx_size);
    TMP107WaitForEcho(uart, tx_size, rx, rx_size);

    // Must wait 1250ms after receiving last device response
    // during address init to write to eeprom.
    Task_sleep(TMP107_AddrInitTimeout);
    return rx[0] & 0xF8;
}

char TMP107LastDevicePoll(UART_Handle uart) {
    // Initialise the chain and return the last device address
        char tx_size = 2;
        char rx_size = 1;
        char tx[2];
        char rx[1];

        tx[0] = 0x55; // Calibration Byte
        tx[1] = 0x57; // AddressInit command code
        TMP107Transmit(uart, tx, tx_size);
        TMP107WaitForEcho(uart, tx_size, rx, rx_size);

        // Must wait 1250ms after receiving last device response
        // during address init to write to eeprom.
        Task_sleep(TMP107_AddrInitTimeout);
        return rx[0] & 0xF8;
}


void setupUART() {

    UART_Handle uartTemp ;
    uartTemp  = TMP107InitUart();
    char boardTempAddr = TMP107Init(uartTemp );
    char motorTempAddr = TMP107LastDevicePoll(uartTemp); // motor_addr var will be a backwards 5 bit
    int device_count = TMP107_Decode5bitAddress(motorTempAddr);

    // Build temperature read command packets
        char tx_size = 3;
        char rx_size = 4;
        char tx[3];
        char rx[4]; // Both TMP107s reply with two packets

        // Setup transmission to make both TMP107s reply with temp
        tx[0] = 0x55; // Calibration Byte
        // The daisy chain returns data starting from the address specified in the command or
        // address phase, and ending with the address of the first device in the daisy chain.
        tx[1] = TMP107_Global_bit | TMP107_Read_bit | motorTempAddr;
        tx[2] = TMP107_Temp_reg;

        while(1) {
            //Semaphore_pend(semTempHandle, BIOS_WAIT_FOREVER);

            // Transmit global temperature read command
            TMP107Transmit(uartTemp, tx, tx_size);
            // Master cannot transmit again until after we've received
            // the echo of our transmit and given the TMP107 adequate
            // time to reply. thus, we wait.
            // Copy the response from TMP107 into user variable
            TMP107WaitForEcho(uartTemp, tx_size, rx, rx_size);

            // Convert two bytes received from TMP107 into degrees C
            float boardTemp = TMP107_DecodeTemperatureResult(rx[3], rx[2]);
            float motorTemp = TMP107_DecodeTemperatureResult(rx[1], rx[0]);
            int x = 0;
//            // Variables for the ring buffer (not quite a ring buffer though)
//            static uint8_t tempHead = 0;
//            boardTempBuffer[tempHead] = boardTemp;
//            motorTempBuffer[tempHead++] = motorTemp;
//            tempHead %= WINDOW_TEMP;

//            // Check if temperature threshold exceeded
//            if (motorTemp > glThresholdTemp) {
//                eStopMotor();
//                eStopGUI();
//            }
        }




//    if (!setupTMP107UART()) {
//        System_abort("Error opening the UART");
//    }
//    System_printf("TMP107 UART Initialized\n");
//    System_flush();
//
//    writeBuffer[0] = 0x55;
//    writeBuffer[1] = 0x95;
//    writeBuffer[2] = 0x5 | TMP107_Encode5bitAddress(0x2); // Set first device address of 2
//
//    TMP107Transmit(writeBuffer, 3);
//    TMP107Receive(readBuffer, 2);
}

