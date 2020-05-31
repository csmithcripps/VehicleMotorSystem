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
#include "driverlib/adc.h"
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
        Task_sleep(100);
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



volatile float boardTemp = 0;
volatile float motorTemp = 0;
volatile float boardTempArray[] = {0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0};
volatile float motorTempArray[] = {0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0};

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

    // Setup transmission packet to get the temperature reading from both sensors
    txBuffer[0] = 0x55; // Calibration Byte (55h)
    txBuffer[1] = TMP107_Global_bit | TMP107_Read_bit | motorTMP107Addr; // Command and address phase
    txBuffer[2] = TMP107_Temp_reg; // Register pointer phase

    while(1) {
        // Transmit global read command
        UART_write(uart, txBuffer, txSize);
        // Receive transmit echos followed by sensor readings
        TMP107_receive(uart, txSize, rxBuffer, rxSize);
        // Convert the sensor readings into degrees C
        motorTemp = TMP107_DecodeTemperatureResult(rxBuffer[1], rxBuffer[0]);
        boardTemp = TMP107_DecodeTemperatureResult(rxBuffer[3], rxBuffer[2]);
        int i;
        // Shift the existing temperature readings
        for (i = 0; i < 29; i++) {
            boardTempArray[i] = boardTempArray[i+1];
            motorTempArray[i] = motorTempArray[i+1];
        }
        // Add the new temperature readings to the arrays
        boardTempArray[29] = boardTemp;
        motorTempArray[29] = motorTemp;
        // sleep task
        Task_sleep(100);
    }
}

uint32_t adcLatestSampleOne;
uint16_t adcLatestSampleTwo;
uint32_t processedGlobal;
uint32_t ui32Count;
uint32_t adcOneRaw[8];
uint32_t adcTwoRaw[8];
uint32_t pui32ADC1Value[8];
uint32_t pui32ADC2Value[8];
uint32_t avgOne = 0;
uint32_t avgTwo = 0;
uint8_t index = 0;

void SetupADC() {
    //
    // Enable the ADC0 module.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    //
    // Wait for the ADC0 module to be ready.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1)){}

    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_7);
    //
    // Enable the first sample sequencer to capture the value of channel 0 when
    // the processor trigger occurs.
    //
    ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceConfigure(ADC1_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);

    ADCSequenceStepConfigure(ADC1_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0);
    ADCSequenceStepConfigure(ADC1_BASE, 1, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH4);

    ADCSequenceEnable(ADC1_BASE, 0);
    ADCSequenceEnable(ADC1_BASE, 1);

    while(1) {
        ADCProcessorTrigger(ADC1_BASE,0);
        ADCProcessorTrigger(ADC1_BASE,1);

        ui32Count = ADCSequenceDataGet(ADC1_BASE,0,pui32ADC1Value);
        ui32Count = ADCSequenceDataGet(ADC1_BASE,1,pui32ADC2Value);

        index = 0;
        for (;index < 8; index++) {
            adcOneRaw[index] = pui32ADC1Value[index];
            adcTwoRaw[index] = pui32ADC2Value[index];
        }

        index = 0;
        adcLatestSampleOne = 0;
        for (; index < 8; index++){
            avgOne += adcOneRaw[index];
            if (index < 4) {
                avgTwo += adcTwoRaw[index];
            }
        }

        avgTwo = avgTwo/8;
        avgOne = avgOne/8;
        adcLatestSampleOne = (avgOne+avgTwo)/2;
        Task_sleep(100);

    }
}
