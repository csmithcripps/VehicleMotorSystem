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
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"

#define MAX_ACC 0.25 // 0->MAX_RPM in ~14 seconds
#define MAX_DEC -0.65 // e-stop: MAX_RPM->0 in ~7 seconds
#define RPM_CHECK_FREQ_TO_SEC 10
#define NUM_RPM_POINTS 29

volatile int rpm = 0;
volatile int hall_count = 0;
volatile int timer_count = 0;
volatile int motor_rpm[] = {0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0,
                            0,0,0,0,0,0,0,0,0,0};

extern bool MotorOn;
extern int duty_screen; // semSpeedLimit
extern float duty_acc;

float duty_error;
float duty_motor;

//*****************************************************************************
// Rotate the Motor (HWI)
//*****************************************************************************
void ISRHall() {
    GPIOIntClear(GPIO_PORTM_BASE, GPIO_PIN_3);
    GPIOIntClear(GPIO_PORTH_BASE, GPIO_PIN_2);
    GPIOIntClear(GPIO_PORTN_BASE, GPIO_PIN_2);
    updateMotor(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3),
                GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2),
                GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2));
    hall_count++;
}

//*****************************************************************************
// Initialize the Motor
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
    // start the motor with normal speed
    setDuty(DUTY_DEFAULT);
}

//*****************************************************************************
// Measure the Motor RPM (Timer)
//*****************************************************************************
void timerRPM() {
    TimerIntClear(TIMER4_BASE, TIMER_BOTH);
    // measure the RPM
    int rot_per_sec = RPM_CHECK_FREQ_TO_SEC * hall_count / HALL_COUNT_PER_REV;
    rpm = SECS_IN_MIN * rot_per_sec;
    hall_count = 0;
    // update the transient factor
    Semaphore_pend(semDutyScreen, BIOS_WAIT_FOREVER);
        if (MotorOn) {
            if (duty_motor < duty_screen) { duty_error =  duty_acc * MAX_ACC; }
            else if (duty_motor > duty_screen) { duty_error = 0.7 * MAX_DEC; } // MAX_RPM->0 in ~10 seconds
            else { duty_error = 0; }
        }
        else if (duty_motor > 0) {
            duty_error = MAX_DEC; // TODO: this should not be at e-stop rate
        }
    Semaphore_post(semDutyScreen);
    // update the motor speed
    Semaphore_pend(semDutyMotor, BIOS_WAIT_FOREVER);
        duty_motor += (float)duty_error;
        if (duty_motor < 0) { duty_motor = DUTY_STOP; }
        setDuty(duty_motor);
    Semaphore_post(semDutyMotor);
    // update the timer counter
    timer_count++;
    int i;
    for (i = 0; i < NUM_RPM_POINTS; i++) { motor_rpm[i] = motor_rpm[i+1]; }
    motor_rpm[NUM_RPM_POINTS] = rpm;
}

//*****************************************************************************
// Starts the Motor (SWI)
//*****************************************************************************
void SWIstartMotor() {
    // set the motor speed
    Semaphore_pend(semDutyMotor, BIOS_WAIT_FOREVER);
        duty_motor = DUTY_KICK;
        setDuty(duty_motor);
    Semaphore_post(semDutyMotor);
    // update the location of the motor
    updateMotor(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3),
                GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2),
                GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2));
}

