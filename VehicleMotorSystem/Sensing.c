/* std Header files */
#include <math.h>
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
#include "driverlib/adc.h"
#include "Drivers/opt3001.h"
#include "Drivers/bmi160.h"
#include "Drivers/globaldefines.h"

#define OPT3001_I2C_ADDRESS             0x47

/* Register addresses */
#define OPT_REG_RESULT                      0x00
#define OPT_REG_CONFIG                      0x01
#define OPT_REG_LOW_LIMIT                   0x02
#define OPT_REG_HIGH_LIMIT                  0x03

extern int AccelerationLimit;
extern int TempLimit; // semTempLimit
extern int CurrentLimit;

int EStopMode;

struct bmi160_dev bmi160Sensor;

I2C_Handle i2c;

volatile int lux = 0;
volatile int luxArray[30] = {0};
volatile float accelArray[30] = {0};

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


/*
 * Function:    readOPT3001
 * ------------------------
 * Reads smoothed data from the ambient light sensor (OPT3001) on the sensor booster pack
 * Saves data in global int array -> luxArray
 */
void readOPT3001(){
    // Declare Variables
    UChar readBuffer[2];            //Buffer to be read into from OPT Sensor
    Bool transferOK;                //Output success/fail: 0/1
    uint16_t  rawData = 0;          //Raw data from sensor
    float     convertedLux = 0;     //Converted data from sensor in LUX
    int i;                       //Loop counters

    // Start task loop
    while(!Semaphore_getCount(semScreenInit)){Task_sleep(10);}
    while(1){
        //Read Sensor
        transferOK = readI2C(OPT3001_I2C_ADDRESS, OPT_REG_RESULT, (uint8_t *)&readBuffer, 2);

        //On Success, store data in sample window
        if (!transferOK) {
            rawData = (readBuffer[0] << 8) | readBuffer[1];
            sensorOpt3001Convert(rawData, &convertedLux);
        }

        //Put sample window data into display data array
        Semaphore_pend(semLUX, BIOS_WAIT_FOREVER);
            // Calculate sliding window average with a window size of 5
            lux = convertedLux;
            for (i = 0; i < 4; i++) { lux += luxArray[29-i]; }
            lux /= 5;

            // Append Sliding window to display Array
            for (i = 0; i < 29; i++) { luxArray[i] = luxArray[i+1]; }
            luxArray[29] = (int) lux;
        Semaphore_post(semLUX);

        Task_sleep(100);
    }
}

void setupOPT3001(I2C_Handle bus){
    UChar writeBuffer[2];
    Bool transferOK;

    writeBuffer[0] = 0xC4;
    writeBuffer[1] = 0x10;
    transferOK = writeI2C(OPT3001_I2C_ADDRESS, OPT_REG_CONFIG, (uint8_t *)&writeBuffer, 2);

    if (transferOK == BMI160_OK){

        System_printf("OPT3001 Initialised\n");
        System_flush();
    }

}

void readBMI160(){
    struct bmi160_sensor_data accel;
    float currentVal;
    float aveAccel;
    uint8_t NSamples = 2;
    uint8_t data_array[9] = { 0 };
    float sampleArray[20] = { 0 };
    uint8_t lsb;
    uint8_t msb;
    int16_t msblsb;
    uint8_t idx = 0;
    int i = 0;

    while(!Semaphore_getCount(semScreenInit)){Task_sleep(10);}
    while(1){
        /* To read 20 points of accel data from FIFO*/
        for(i=0;i<NSamples;i++){
            if (bmi160_get_regs(BMI160_ACCEL_DATA_ADDR, data_array, 6, &bmi160Sensor) == BMI160_OK)
            {
                /* Extract Accel Data */
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
            for (i = 0; i < NSamples-1; i++) { sampleArray[i] = sampleArray[i+1]; }
            sampleArray[NSamples-1] = currentVal;
        }
        Task_sleep(20);

        //Take Average of sample window
        aveAccel = 0;
        for (i = 0; i < NSamples; i++) { aveAccel += sampleArray[i]; }
        aveAccel /= NSamples;

        //Put sample window data into display data array
        Semaphore_pend(semLUX, BIOS_WAIT_FOREVER);
            for (i = 0; i < 29; i++) { accelArray[i] = accelArray[i+1]; }
            accelArray[29] = aveAccel;
        Semaphore_post(semLUX);
        if (aveAccel>AccelerationLimit){
            EStopMode = 3;
            Swi_post(EStop);
        }
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



volatile float boardTemp = 0;
volatile float motorTemp = 0;
volatile float boardTempArray[30] = {0};
volatile float motorTempArray[30] = {0};

//*****************************************************************************
// Initialises the UART for use with the TMP107 sensor array
//*****************************************************************************
UART_Handle TMP107_InitUart() {
    // Declare and configure relevant UART params
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.readMode = UART_MODE_BLOCKING;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readTimeout = TMP107_Timeout*2;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 115200;
    // Open the UART and receive a handle
    UART_Handle uart = UART_open(Board_UART7, &uartParams);
    // Throw system abort if there is an error opening the UART
    if (uart == NULL) {
        System_abort("Error opening the UART");
    }
    // Return the UART handle
    return uart;
}

//*****************************************************************************
// Receives the transmit echo followed by the TMP107 sensor array data
//*****************************************************************************
void TMP107_receive(UART_Handle uart, char txSize, char* rxBuffer, char rxSize){
    // define the stop bit value
    int stopBit = 1;
    // Define the count of bytes to receive
    char rxCount = txSize + rxSize + stopBit;

    // Temporary  buffer
    char rxTmp[32];
    // Fill the temporary buffer with the UART queue
    UART_read(uart, &rxTmp, rxCount);

    // Copy the bytes received from the TMP107 sensor array to the rxBuffer
    char i;
    if (rxSize > 0) {
        for (i = 0; i < rxSize; i++) {
            rxBuffer[i] = rxTmp[txSize + i];
        }
    }
}

//*****************************************************************************
// Configures the addresses of the TMP107 sensors
//*****************************************************************************
char TMP107_Init(UART_Handle uart) {
    // Initialise transmit and receive buffers
    char txSize = 3;
    char rxSize = 1;
    char txBuffer[3];
    char rxBuffer[1];

    // Setup transmission packet to set the addresses of the sensors
    txBuffer[0] = 0x55; // Calibration Byte (55h)
    txBuffer[1] = 0x95;  // Address initialise command
    txBuffer[2] = 0x5 | TMP107_Encode5bitAddress(0x1); // Set the first sensor to address 1
    // Transmit address initialise command
    UART_write(uart, txBuffer, txSize);
    // Receive the address of the first sensor
    TMP107_receive(uart, txSize, rxBuffer, rxSize);
    // Wait for the communication interface in the TMP107 to reset
    Task_sleep(TMP107_AddrInitTimeout);
    // Return the address of the first sensor in the chain
    return rxBuffer[0] & 0xF8;
}

//*****************************************************************************
// Gets the address of the last TMP sensor in the array
//*****************************************************************************
char TMP107_LastDevicePoll(UART_Handle uart) {
    // Initialise transmit and receive buffers
    char txSize = 2;
    char rxSize = 1;
    char txBuffer[2];
    char rxBuffer[1];

    // Setup transmission packet to get the address of the last sensor
    txBuffer[0] = 0x55; // Calibration Byte (55h)
    txBuffer[1] = 0x57; // Last device poll command
    // Transmit last device poll command
    UART_write(uart, txBuffer, txSize);
    // Receive the address of the last TMP107 sensor
    TMP107_receive(uart, txSize, rxBuffer, rxSize);
    // Return the address of the last sensor in the chain
    return rxBuffer[0] & 0xF8;
}

//*****************************************************************************
// The task for initialising, configuring and reading the TMP107 sensors
//*****************************************************************************
void readTMP107() {
    // Initialise the UART and receive the handle
    UART_Handle uart = TMP107_InitUart();
    // Initialise addresses of the TMP107 sensors
    char boardTMP107Addr = TMP107_Init(uart);
    // Retreive the address of the last TMP107 sensor using Last Device Poll
    char motorTMP107Addr = TMP107_LastDevicePoll(uart);

    // Initialise transmit and receive buffers
    char txSize = 3;
    char rxSize = 4;
    char txBuffer[3];
    char rxBuffer[4];

    int i;
    float aveMotorTemp[10] = {0};
    float aveBoardTemp[10] = {0};

    // Setup transmission packet to get the temperature reading from both sensors
    txBuffer[0] = 0x55; // Calibration Byte (55h)
    txBuffer[1] = TMP107_Global_bit | TMP107_Read_bit | motorTMP107Addr; // Command and address phase
    txBuffer[2] = TMP107_Temp_reg; // Register pointer phase

    System_printf("TMP107 Initialised\n");
    System_flush();

    while(!Semaphore_getCount(semScreenInit)){Task_sleep(10);}
    while(1) {

            // Transmit global read command
            UART_write(uart, txBuffer, txSize);
            // Receive transmit echos followed by sensor readings
            TMP107_receive(uart, txSize, rxBuffer, rxSize);

            for (i = 0; i < 9; i++) {
                aveMotorTemp[i] = aveMotorTemp[i+1];
                aveBoardTemp[i] = aveBoardTemp[i+1];
            }
            aveMotorTemp[9] = TMP107_DecodeTemperatureResult(rxBuffer[1], rxBuffer[0]);
            aveBoardTemp[9] = TMP107_DecodeTemperatureResult(rxBuffer[3], rxBuffer[2]);

            boardTemp = 0;
            motorTemp = 0;
            for (i = 0; i < 10; i++) {
                boardTemp += aveMotorTemp[i];
                motorTemp += aveBoardTemp[i];
            }
            boardTemp /= 10;
            motorTemp /= 10;

            Semaphore_pend(semTEMP, BIOS_WAIT_FOREVER);

            // Shift the existing temperature readings
            for (i = 0; i < 29; i++) {
                boardTempArray[i] = boardTempArray[i+1];
                motorTempArray[i] = motorTempArray[i+1];
            }
            // Add the new temperature readings to the arrays
            boardTempArray[29] = boardTemp;
            motorTempArray[29] = motorTemp;
            Semaphore_post(semTEMP);

            if  (motorTemp>TempLimit && MotorKit){
                EStopMode = 2;
                Swi_post(EStop);
            }
            // sleep task
            Task_sleep(100);
        }
}

float getCurrent(uint32_t sensorVoltage) {
    float voltage = (float)sensorVoltage * 3.3 / 0xFFF;
    return ((MOTOR_REFERENCE_VOLTAGE / 2 - voltage) / (G_CSA * R_SENSE));
}

volatile float motorPower;
volatile float motorPowerArray[30] = {0};

void SetupADC() {
    int i, j;
    float iSensor1;
    float iSensor2;
    float iSensor;
    float totalCurrentAve;
    float avePow[10] = {0};
    uint32_t pui32ADC1Value;
    uint32_t pui32ADC2Value;

    // Enable the ADC0 module.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    // Wait for the ADC0 module to be ready.
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1)) { }

    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7);
    // Enable the first sample sequencer to capture the value of channel 0 when
    // the processor trigger occurs.
    ADCSequenceConfigure(ADC1_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceConfigure(ADC1_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);

    ADCSequenceStepConfigure(ADC1_BASE, 1, 0, ADC_CTL_IE | ADC_CTL_CH0 | ADC_CTL_END);
    ADCSequenceStepConfigure(ADC1_BASE, 2, 0, ADC_CTL_IE | ADC_CTL_CH4 | ADC_CTL_END);

    ADCSequenceEnable(ADC1_BASE, 1);
    ADCSequenceEnable(ADC1_BASE, 2);

    ADCIntClear(ADC1_BASE, 1);
    ADCIntClear(ADC1_BASE, 2);

    while(!Semaphore_getCount(semScreenInit)){Task_sleep(10);}
    while(1) {
            for (j = 0; j < 25; j++) {
                ADCProcessorTrigger(ADC1_BASE, 1);
                ADCProcessorTrigger(ADC1_BASE, 2);

                // Wait until the sample sequence has completed.
                while(!ADCIntStatus(ADC1_BASE, 1, false) || !ADCIntStatus(ADC1_BASE, 2, false)) { }

                ADCIntClear(ADC1_BASE, 1);
                ADCIntClear(ADC1_BASE, 2);

                ADCSequenceDataGet(ADC1_BASE, 1, &pui32ADC1Value);
                ADCSequenceDataGet(ADC1_BASE, 2, &pui32ADC2Value);

                iSensor1 = getCurrent(pui32ADC1Value);
                iSensor2 = getCurrent(pui32ADC2Value);

                totalCurrentAve = (fabs(iSensor1) + fabs(iSensor2)) * 3/2;

                iSensor  = 24 * totalCurrentAve;

                for (i = 0; i < 9; i++) { avePow[i] = avePow[i+1]; }
                avePow[9] = iSensor;

                if (totalCurrentAve>CurrentLimit){
                    EStopMode = 1;
                    Swi_post(EStop);
                }

                Task_sleep(4);
            }
            motorPower = 0;
            for (i = 0; i < 10; i++) { motorPower += avePow[i]; }
            motorPower /= 10;

            Semaphore_pend(semPower, BIOS_WAIT_FOREVER);
            int i;
            // Shift the existing power readings
            for (i = 0; i < 29; i++) {
                motorPowerArray[i] = motorPowerArray[i+1];
            }
            // Add the new temperature readings to the arrays
            motorPowerArray[29] = motorPower;
            Semaphore_post(semPower);
        }
}
