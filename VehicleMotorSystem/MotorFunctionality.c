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
// Hardware Interrupt - triggered by the Hall Effect Sensors (A, B and C)
//*****************************************************************************
void ISRHall() {
    Event_post(updateMotor, Event_Id_00);
}

//*****************************************************************************
// Initialise the Motor
//*****************************************************************************
void initMotor() {
    // Define the interrupt pins
    GPIOPinTypeGPIOInput(GPIO_PORTM_BASE, GPIO_PIN_3); // Hall A
    GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_2); // Hall B
    GPIOPinTypeGPIOInput(GPIO_PORTN_BASE, GPIO_PIN_2); // Hall C
    // Interrupt for hall sensor pins
    GPIOPadConfigSet(GPIO_PORTM_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    // Set interrupt on both rising and falling edge
    GPIOIntTypeSet(GPIO_PORTM_BASE, GPIO_PIN_3, GPIO_BOTH_EDGES);
    GPIOIntTypeSet(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_BOTH_EDGES);
    GPIOIntTypeSet(GPIO_PORTN_BASE, GPIO_PIN_2, GPIO_BOTH_EDGES);
    // Enable the pins
    GPIOIntEnable(GPIO_PORTM_BASE, GPIO_PIN_3);
    GPIOIntEnable(GPIO_PORTH_BASE, GPIO_PIN_2);
    GPIOIntEnable(GPIO_PORTN_BASE, GPIO_PIN_2);
    IntMasterEnable();
    // Enable motor
    Error_Block eb;
    Error_init(&eb);
    initMotorLib(50, &eb); // set the frequency of the PWM (1/50e-6) = 20Khz
    if (!&eb) { System_printf("Unsuccessfully initialized the motor.\n"); }
    else { System_printf("Successfully initialized the motor.\n"); }
    System_flush();
    enableMotor();
    setDuty(25);
    updateMotor(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3),
                GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2),
                GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2));
    UInt motor_post_check;
    while (1) {
        motor_post_check = Event_pend(updateMotor, Event_Id_NONE, Event_Id_00, BIOS_WAIT_FOREVER);
        if (motor_post_check) {
            updateMotor(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3),
                        GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2),
                        GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2));
        }
    }
}
