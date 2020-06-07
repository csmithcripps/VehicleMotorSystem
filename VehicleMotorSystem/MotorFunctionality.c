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

#define RPM_ACC 500 // 0->MAX_RPM in ~14 seconds
#define RPM_DEC -500 // e-stop: MAX_RPM->0 in ~7 seconds
#define RPM_CHECK_FREQ_TO_SEC 100 // checking 100 times a second
#define NUM_RPM_POINTS 29

volatile int hall_count = 0;
volatile int timer_count = 0;
volatile int motor_rpm[30] = {0};
volatile int desired_motor_rpm[30] = {0};

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
    setDuty(DUTY_MAX/2);
}

extern bool MotorOn;
extern int rpm_screen; // semSpeedLimit

bool EStopActive = false;

float rpm_desired = 0;
float duty_motor;
float rpm_buffer[10] = {0};
float duty_buffer[100] = {0};
float error_buffer[100] = {0};

float error_rpm;
float error_acc_rpm;
//*****************************************************************************
// Measure the Motor RPM (Timer): called 100 Hz
//*****************************************************************************
void timerRPM() {
    TimerIntClear(TIMER4_BASE, TIMER_BOTH);
    // measure the RPM
    int rot_per_sec = RPM_CHECK_FREQ_TO_SEC * hall_count / HALL_COUNT_PER_REV;
    int rpm = SECS_IN_MIN * rot_per_sec;
    hall_count = 0;
    int i;
    // write to the buffers
    for (i = 0; i < 9; i++) { rpm_buffer[i] = rpm_buffer[i+1]; }
    rpm_buffer[9] = rpm;
    // calculate the average RPM
    int rpm_ave = 0;
    for (i = 0; i < 10; i++) { rpm_ave += rpm_buffer[i]; }
    rpm_ave /= 10;
    // set the desired RPM
    Semaphore_pend(semDutyScreen, BIOS_WAIT_FOREVER);

        if (EStopActive) { rpm_desired += 2*RPM_DEC / 100; }
        else if (MotorOn) {
            // acceleration
            if (rpm_desired < rpm_screen) { rpm_desired += RPM_ACC / 100; }
            // deceleration
            else if (rpm_desired > rpm_screen) { rpm_desired += RPM_DEC / 100; }
        }
        // decelerate
        else if (rpm_desired > 0) { rpm_desired += RPM_DEC / 100; }
        if (rpm_desired < 0) { rpm_desired = 0; }
    Semaphore_post(semDutyScreen);
    // calculate the error in our RPM acceleration
    error_rpm = rpm_desired - (float)(rpm_ave);
    // update the motor speed
    Semaphore_pend(semDutyMotor, BIOS_WAIT_FOREVER);
        duty_motor += 0.0001 * error_rpm;
        if (duty_motor < DUTY_STOP) { duty_motor = DUTY_STOP; }
        if (duty_motor > DUTY_MAX) { duty_motor = DUTY_MAX; }

        // append to the history of the motor duty cycle
        for (i = 0; i < 99; i++) { duty_buffer[i] = duty_buffer[i+1]; }
        duty_buffer[99] = duty_motor;
        for (i = 0; i < 99; i++) { error_buffer[i] = error_buffer[i+1]; }
        error_buffer[99] = error_rpm;

        // set the motor duty cycle
        setDuty((int16_t)duty_motor);
    Semaphore_post(semDutyMotor);
    Semaphore_pend(semRPM, BIOS_WAIT_FOREVER);
        for (i = 0; i < NUM_RPM_POINTS; i++) {
            motor_rpm[i] = motor_rpm[i+1];
            desired_motor_rpm[i] = desired_motor_rpm[i+1];
        }
        motor_rpm[NUM_RPM_POINTS] = rpm;
        desired_motor_rpm[NUM_RPM_POINTS] = rpm_desired;
    Semaphore_post(semRPM);
    // update the timer counter
    timer_count++;
}

//*****************************************************************************
// Starts the Motor (SWI)
//*****************************************************************************
void SWIstartMotor() {
    // set the motor speed
    Semaphore_pend(semDutyMotor, BIOS_WAIT_FOREVER);
        rpm_desired += 300;
        duty_motor = DUTY_KICK;
        setDuty(duty_motor);
    Semaphore_post(semDutyMotor);
    // update the location of the motor
    updateMotor(GPIOPinRead(GPIO_PORTM_BASE, GPIO_PIN_3),
                GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_2),
                GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_2));
}



extern tContext sContext;
bool EStopMotor(){
    return 0;
}

extern int EStopMode;
void SWI_EStop() {
    tRectangle sRect;
    char tempStr[40];
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext);
    sRect.i16YMax = GrContextDpyHeightGet(&sContext);
    GrContextForegroundSet(&sContext, ClrRed);
    GrRectFill(&sContext, &sRect);

    sRect.i16XMin = 10;
    sRect.i16YMin = 10;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext)-10;
    sRect.i16YMax = GrContextDpyHeightGet(&sContext)-10;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);

    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextFontSet(&sContext, &g_sFontCm20);
    sprintf(tempStr, "E-Stop (Mode %d) Engaged.", EStopMode);
    GrStringDraw(&sContext, tempStr, -1, 15, 20, 0);
    sprintf(tempStr, "Reset to Continue.");
    GrStringDraw(&sContext, tempStr, -1, 15, 40, 0);

    EStopActive = true;
    while(1);
}
