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
#include "driverlib/uart.h"
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


    //Init OPT3001
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
