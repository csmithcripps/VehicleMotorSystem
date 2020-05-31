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
//#include "Drivers/i2cOptDriver.h"
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
#include "Drivers/bmi160.h"

#define OPT3001_I2C_ADDRESS             0x47

/* Register addresses */
#define OPT_REG_RESULT                      0x00
#define OPT_REG_CONFIG                      0x01
#define OPT_REG_LOW_LIMIT                   0x02
#define OPT_REG_HIGH_LIMIT                  0x03


struct bmi160_dev bmi160Sensor;

I2C_Handle i2c;

volatile int lux = 0;
volatile int luxArray[] = {0,0,0,0,0,0,0,0,0,0,
                           0,0,0,0,0,0,0,0,0,0,
                           0,0,0,0,0,0,0,0,0,0};
volatile int accelArray[] = {0,0,0,0,0,0,0,0,0,0,
                             0,0,0,0,0,0,0,0,0,0,
                             0,0,0,0,0,0,0,0,0,0};

int readI2C(uint8_t addr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    I2C_Transaction i2cTransactionRead;
    UChar writeBuffer[1];
    Bool transferOK;

    i2cTransactionRead.slaveAddress = addr; /* 7-bit peripheral slave address */
    i2cTransactionRead.writeBuf = writeBuffer; /* Buffer to be written */
    i2cTransactionRead.writeCount = 1; /* Number of bytes to be written */
    i2cTransactionRead.readBuf = data; /* Buffer to be read */
    i2cTransactionRead.readCount = len; /* Number of bytes to be read */
    writeBuffer[0] = reg_addr;

    Semaphore_pend(semI2C, BIOS_WAIT_FOREVER);
    transferOK = I2C_transfer(i2c, &i2cTransactionRead); /* Perform I2C transfer */
    Semaphore_post(semI2C);
    if(transferOK) return 0;
    return 1;

}

int writeI2C(uint8_t addr, uint8_t reg_addr, uint8_t *data, uint16_t len){
    I2C_Transaction i2cTransactionWrite;
    UChar writeBuffer[100];
    Bool transferOK;
    int i;
    for (i=0;i<len;i++){
        writeBuffer[i+1] = *(data+i);
    }
    writeBuffer[0] = reg_addr;

    i2cTransactionWrite.slaveAddress = addr; /* 7-bit peripheral slave address */
    i2cTransactionWrite.writeBuf = writeBuffer; /* Buffer to be written */
    i2cTransactionWrite.writeCount = len+1; /* Number of bytes to be written */
    i2cTransactionWrite.readBuf = NULL; /* Buffer to be read */
    i2cTransactionWrite.readCount = 0; /* Number of bytes to be read */

    Semaphore_pend(semI2C, BIOS_WAIT_FOREVER);
    transferOK = I2C_transfer(i2c, &i2cTransactionWrite); /* Perform I2C transfer */
    Semaphore_post(semI2C);

    if(transferOK) return 0;
    return 1;

}

void readOPT3001(){
    UChar readBuffer[2];
    Bool transferOK;

    uint16_t  rawData = 0;
    float     convertedLux = 0;
    while(1){
        transferOK = readI2C(OPT3001_I2C_ADDRESS, OPT_REG_RESULT, (uint8_t *)&readBuffer, 2);
        rawData = (readBuffer[0] << 8) | readBuffer[1];

        if (!transferOK) {
            sensorOpt3001Convert(rawData, &convertedLux);
            lux = (int) convertedLux;
            int i;
            for (i = 0; i < 29; i++) { luxArray[i] = luxArray[i+1]; }
            luxArray[29] = lux;
        }
        Task_sleep(300);
    }
}


void setupOPT3001(I2C_Handle bus){
    UChar writeBuffer[2];
    Bool transferOK;

    writeBuffer[0] = 0xC4;
    writeBuffer[1] = 0x10;
    transferOK = writeI2C(OPT3001_I2C_ADDRESS, OPT_REG_CONFIG, (uint8_t *)&writeBuffer, 2);

    if (!transferOK){

        System_printf("OPT3001 Initialised\n");
        System_flush();
    }

}


void readBMI160(){
    struct bmi160_sensor_data accel;
    int currentVal;
    uint8_t data_array[9] = { 0 };
    uint8_t FIFO_Length;
    uint8_t lsb;
    uint8_t msb;
    int16_t msblsb;
    uint8_t idx = 0;
    int i = 0;

    while(1){
        /* To read only Accel data */
        for(i=0;i<30;i++){
            if (bmi160_get_regs(BMI160_FIFO_DATA_ADDR, &data_array, 6, &bmi160Sensor) == BMI160_OK)
            {
                /* Accel Data */
                idx = 0;
                lsb = data_array[idx++];
                msb = data_array[idx++];
                msblsb = (int16_t)((msb << 8) | lsb);
                accel.x = msblsb; /* Data in X axis */
                lsb = data_array[idx++];
                msb = data_array[idx++];
                msblsb = (int16_t)((msb << 8) | lsb);
                accel.y = msblsb; /* Data in Y axis */
                lsb = data_array[idx++];
                msb = data_array[idx++];
                msblsb = (int16_t)((msb << 8) | lsb);
                accel.z = msblsb; /* Data in Z axis */

                accel.sensortime = 0;
            }
            currentVal = 9.81*2*  (abs(accel.x) + abs(accel.y) + abs(accel.z))/(0xFFFF/2);
            accelArray[i] = currentVal-10;
        }
        currentVal = accelArray[29];
        Task_sleep(150);
    }
}

void setupBMI160(I2C_Handle bus){

    bmi160Sensor.id = BMI160_I2C_ADDR;
    bmi160Sensor.interface = BMI160_I2C_INTF;
    bmi160Sensor.read = (bmi160_com_fptr_t)&readI2C;
    bmi160Sensor.write = (bmi160_com_fptr_t)&writeI2C;
    bmi160Sensor.delay_ms = &Task_sleep;

    if(bmi160_init(&bmi160Sensor) != BMI160_OK){
        System_printf("BMI160 Setup Failed\n");
        System_flush();
        return;
    }

    bmi160Sensor.accel_cfg.odr = BMI160_ACCEL_ODR_200HZ;
    bmi160Sensor.accel_cfg.range = BMI160_ACCEL_RANGE_2G;
    bmi160Sensor.accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;

    /* Select the power mode of accelerometer sensor */
    bmi160Sensor.accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;
    if(bmi160_set_sens_conf(&bmi160Sensor) != BMI160_OK){
        System_printf("BMI160 Config Acc Failed\n");
        System_flush();
        return;
    }

    uint8_t writeBuffer[1];
    writeBuffer[0] = BMI160_FIFO_ACCEL | 0;
    if(writeI2C(BMI160_I2C_ADDR, BMI160_FIFO_CONFIG_1_ADDR, writeBuffer, 1) != BMI160_OK){
        System_printf("BMI160 FIFO Config Failed\n");
        System_flush();
        return;
    }

    System_printf("BMI160 Initialised\n");
    System_flush();

}

void setupI2C(){
    // I2C Parameters
    UInt peripheralNum = 0; /* Such as I2C0 */
    I2C_Params i2cParams;

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
    System_printf("I2C Initialised\n");
    System_flush();

    setupOPT3001(i2c);
    setupBMI160(i2c);
}
