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
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"

//*****************************************************************************
// Global variables
//*****************************************************************************
UART_Handle uart;
int SpeedLimit;         // desired speed
int AccelerationLimit;  // max acceleration
int CurrentLimit;       // max current
int TempLimit;          // max temperature

//*****************************************************************************
// Initialize the UART connection
//*****************************************************************************
void initUART() {
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.baudRate = 115200;
    uart = UART_open(Board_UART0, &uartParams);
    if (uart == NULL) { System_abort("Error opening the UART"); }
    System_printf("UART Initialized\n");
    System_flush();
}

//*****************************************************************************
// Main
//*****************************************************************************
int main(void) {
    System_printf("Starting system initialization\n");
    System_flush();
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initI2C();
    Board_initUART();
    initUART();
    System_printf(  "System initialization successful\n"
                    "Starting vehicle motor system\n");
    System_flush();
    BIOS_start();
    return 0;
}
