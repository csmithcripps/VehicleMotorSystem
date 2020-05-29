#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>
#include <xdc/cfg/global.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"
#include "Drivers/ustdlib.h"
#include "Drivers/globaldefines.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"
#include "Board.h"
#include "driverlib/interrupt.h"

//*****************************************************************************
// Forward declarations for the globals required to define the widgets at
// compile-time.
//*****************************************************************************
void OnPrevious(tWidget *psWidget);
void OnNext(tWidget *psWidget);
void OnStartStop(tWidget *psWidget);
void Start();
void Stop();
void OnSliderChange(tWidget *psWidget, int32_t i32Value);
void OnPanel1(tWidget *psWidget, tContext *psContext);
void OnPanel2(tWidget *psWidget, tContext *psContext);
extern tCanvasWidget g_psPanels[];

//*****************************************************************************
// Shared resources that use semaphores
//*****************************************************************************
time_t t; // semTime
struct tm *tm;

extern int lux;
extern int rpm;
extern int motor_rpm[];
extern int AccelerationLimit; // semAccelerationLimit
extern int CurrentLimit; // semCurrentLimit
extern int TempLimit; // semTempLimit

//*****************************************************************************
// Text display after and next-to the sliders
//*****************************************************************************
Canvas(g_sSliderValueCanvas4, g_psPanels, 0, 0,
       &g_sKentec320x240x16_SSD2119, 1, 158, 95, 40,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm18, "Temperature", 0, 0);
Canvas(g_sSliderValueCanvas3, g_psPanels, &g_sSliderValueCanvas4, 0,
       &g_sKentec320x240x16_SSD2119, 1, 116, 95, 40,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm18, "Current", 0, 0);
Canvas(g_sSliderValueCanvas2, g_psPanels, &g_sSliderValueCanvas3, 0,
       &g_sKentec320x240x16_SSD2119, 1, 74, 95, 40,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm18, "Acceleration", 0, 0);
Canvas(g_sSliderValueCanvas1, g_psPanels, &g_sSliderValueCanvas2, 0,
       &g_sKentec320x240x16_SSD2119, 1, 32, 95, 40,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm18, "Motor Speed", 0, 0);
Canvas(g_sPanel1, g_psPanels, &g_sSliderValueCanvas1, 0,
       &g_sKentec320x240x16_SSD2119, 0, 0, 0, 0,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm18, "", 0, 0);

//*****************************************************************************
// Sliders
//*****************************************************************************
tSliderWidget g_psSliders[] = {
    SliderStruct(  g_psPanels, g_psSliders + 1, 0, &g_sKentec320x240x16_SSD2119,
                   105, 35, 190, 30, 0, 100, 50,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "50%", 0, 0, OnSliderChange ),
    SliderStruct(  g_psPanels, g_psSliders + 2, 0, &g_sKentec320x240x16_SSD2119,
                   105, 77, 190, 30, 0, 100, 50,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "50%", 0, 0, OnSliderChange ),
    SliderStruct(  g_psPanels, g_psSliders + 3, 0, &g_sKentec320x240x16_SSD2119,
                   105, 119, 190, 30, 0, 100, 50,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "50%", 0, 0, OnSliderChange),
    SliderStruct(  g_psPanels, &g_sPanel1, 0, &g_sKentec320x240x16_SSD2119,
                   105, 161, 190, 30, 0, 100, 50,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "50%", 0, 0, OnSliderChange),
};

//*****************************************************************************
// The second panel
//*****************************************************************************
Canvas(g_sPanel2, g_psPanels + 1, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnPanel2);

//*****************************************************************************
// An array of canvas widgets, one per panel. Each canvas is filled with
// black, overwriting the contents of the previous panel.
//*****************************************************************************
tCanvasWidget g_psPanels[] = {
       CanvasStruct(0, 0, &g_psSliders, &g_sKentec320x240x16_SSD2119,
                    0, 24, 320, 176, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
       CanvasStruct(0, 0, &g_sPanel2, &g_sKentec320x240x16_SSD2119,
                    0, 24, 320, 176, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

void chooseTest(tWidget *psWidget) {
    GPIO_toggle(Board_LED0);
}

RectangularButton(g_sChooseMotor, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 40, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Motor", 0, 0, 0, 0, chooseTest);

RectangularButton(g_sChooseLight, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 80, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Light", 0, 0, 0, 0, chooseTest);

RectangularButton(g_sChooseTemp, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 120, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Temp.", 0, 0, 0, 0, chooseTest);

RectangularButton(g_sChooseAcc, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 160, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Acc.", 0, 0, 0, 0, chooseTest);

//*****************************************************************************
// The buttons at the bottom of (all) screen(s).
//*****************************************************************************
RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 10, 200, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_AUTO_REPEAT),
                  ClrBlack, ClrBlack, ClrWhite, ClrWhite, &g_sFontCm20,
                  "-", 0, 0, 0, 0, OnPrevious);

RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 200, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "+", 0, 0, 0, 0, OnNext);

RectangularButton(g_sStartStop, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 105, 200, 110, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrLimeGreen, ClrGreen, ClrWhite, ClrWhite, &g_sFontCm20b,
                  "Start", 0, 0, 0, 0, OnStartStop);

//*****************************************************************************
// The panel that is currently being displayed.
//*****************************************************************************
uint32_t g_ui32Panel;

volatile int display_page;
//*****************************************************************************
// Handles presses of the previous panel button.
//*****************************************************************************
void OnPrevious(tWidget *psWidget) {
    display_page = 1; // display the first page
    // There is nothing to be done if the first panel is already being
    // displayed.
    if(g_ui32Panel == 0) { return; }
    // Remove the current panel.
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    // Decrement the panel index.
    g_ui32Panel--;
    // Add and draw the new panel.
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));
    // See if this is the first panel.
    if(g_ui32Panel == 0) {
        // Clear the previous button from the display since the first panel is
        // being displayed.
        PushButtonTextOff(&g_sPrevious);
        PushButtonOutlineOff(&g_sPrevious);
        PushButtonFillColorSet(&g_sPrevious, ClrBlack);
        PushButtonFillColorPressedSet(&g_sPrevious, ClrBlack);
        WidgetPaint((tWidget *)&g_sPrevious);
    }
    // See if the previous panel was the last panel.
    if(g_ui32Panel == 0) {
        // Display the next button.
        PushButtonTextOn(&g_sNext);
        PushButtonOutlineOn(&g_sNext);
        PushButtonFillColorSet(&g_sNext, ClrDarkBlue);
        PushButtonFillColorPressedSet(&g_sNext, ClrBlue);
        WidgetPaint((tWidget *)&g_sNext);
    }
}

//*****************************************************************************
// Handles presses of the next panel button. // TODO: Mark
//*****************************************************************************
void OnNext(tWidget *psWidget) {
    display_page = 2; // display the first page
    // There is nothing to be done if the last panel is already being
    // displayed.
    if(g_ui32Panel == 1) { return; }
    // Remove the current panel.
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));
    // Increment the panel index.
    g_ui32Panel++;
    // Add and draw the new panel.
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));
    // See if the previous panel was the first panel.
    if(g_ui32Panel == 1) {
        // Display the previous button.
        PushButtonTextOn(&g_sPrevious);
        PushButtonOutlineOn(&g_sPrevious);
        PushButtonFillColorSet(&g_sPrevious, ClrDarkBlue);
        PushButtonFillColorPressedSet(&g_sPrevious, ClrBlue);
        WidgetPaint((tWidget *)&g_sPrevious);
    }
    // See if this is the last panel.
    if(g_ui32Panel == 1) {
        // Clear the next button from the display since the last panel is being displayed.
        PushButtonTextOff(&g_sNext);
        PushButtonOutlineOff(&g_sNext);
        PushButtonFillColorSet(&g_sNext, ClrBlack);
        PushButtonFillColorPressedSet(&g_sNext, ClrBlack);
        WidgetPaint((tWidget *)&g_sNext);
    }
}

volatile bool MotorOn = false;
//*****************************************************************************
// Handles presses of the start/stop button.
//*****************************************************************************
void OnStartStop(tWidget *psWidget) {
    if (MotorOn) { Stop(); }
    else { Start(); }
}

#define MOTOR_SPEED_SLIDER              0
#define ALLOWABLE_ACCELLERATION_SLIDER  1
#define CURRENT_LIMIT_SLIDER            2
#define TEMPERATURE_LIMIT_SLIDER        3

volatile int duty_screen = DUTY_DEFAULT;
volatile float duty_acc = 0.5;
//*****************************************************************************
// Handles changes to the sliders.
//*****************************************************************************
void OnSliderChange(tWidget *psWidget, int32_t i32Value) {
    static char pcSpeedText[5];
    static char pcAccelText[5];
    static char pcAmpereText[5];
    static char pcTempText[5];

    if(psWidget == (tWidget *)&g_psSliders[MOTOR_SPEED_SLIDER]) {
        Semaphore_pend(semDutyScreen, BIOS_WAIT_FOREVER);
            duty_screen = (int)round(DUTY_MAX * ((float)i32Value / 100));
        Semaphore_post(semDutyScreen);

        usprintf(pcSpeedText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[MOTOR_SPEED_SLIDER], pcSpeedText);
        WidgetPaint((tWidget *)&g_psSliders[MOTOR_SPEED_SLIDER]);
    }
    else if(psWidget == (tWidget *)&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER]) {
        Semaphore_pend(semDutyScreen, BIOS_WAIT_FOREVER);
            duty_acc = (float)i32Value / 100;
        Semaphore_post(semDutyScreen);

        usprintf(pcAccelText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER], pcAccelText);
        WidgetPaint((tWidget *)&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER]);
    }
    else if(psWidget == (tWidget *)&g_psSliders[CURRENT_LIMIT_SLIDER]) {
        usprintf(pcAmpereText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[CURRENT_LIMIT_SLIDER], pcAmpereText);
        WidgetPaint((tWidget *)&g_psSliders[CURRENT_LIMIT_SLIDER]);
    }
    else if(psWidget == (tWidget *)&g_psSliders[TEMPERATURE_LIMIT_SLIDER]) {
        usprintf(pcTempText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[TEMPERATURE_LIMIT_SLIDER], pcTempText);
        WidgetPaint((tWidget *)&g_psSliders[TEMPERATURE_LIMIT_SLIDER]);
    }
}

//*****************************************************************************
// Handles paint requests for the introduction canvas widget.
//*****************************************************************************
void OnPanel1(tWidget *psWidget, tContext *psContext) {
    // nothing
}

//*****************************************************************************
// Handles paint requests for the primitives canvas widget.
//*****************************************************************************
void OnPanel2(tWidget *psWidget, tContext *psContext) {
    // nothing
}

//*****************************************************************************
// Start the motor.
//*****************************************************************************
void Start() {
    MotorOn = true;
    GPIO_write(Board_LED0, Board_LED_ON);
    PushButtonTextSet(&g_sStartStop, "Stop");
    PushButtonFillColorSet(&g_sStartStop, ClrRed);
    PushButtonFillColorPressedSet(&g_sStartStop, ClrDarkRed);
    WidgetPaint((tWidget *)&g_sStartStop);
    Swi_post(motorStart);
}

//*****************************************************************************
// Stop the motor.
//*****************************************************************************
void Stop() {
    MotorOn = false;
    GPIO_write(Board_LED0, Board_LED_OFF);
    PushButtonTextSet(&g_sStartStop, "Start");
    PushButtonFillColorSet(&g_sStartStop, ClrLimeGreen);
    PushButtonFillColorPressedSet(&g_sStartStop, ClrGreen);
    WidgetPaint((tWidget *)&g_sStartStop);
}

//*****************************************************************************
// Timer that triggers the screen update.
//*****************************************************************************
bool updateTime;
void UpdateTime() {
    while(1) {
        Semaphore_pend(semTime, BIOS_WAIT_FOREVER);
            updateTime = true;
        Semaphore_post(semTime);
        Task_sleep(1000);
    }
}

//*****************************************************************************
// Main display functionality.
//*****************************************************************************
void UiStart() {
    tContext sContext;
    tRectangle sRect;
    tRectangle sRect_rpm;
    char tempStr[40];
    bool day = true;
    Types_FreqHz cpuFreq;
    BIOS_getCpuFreq(&cpuFreq);

    t = time(NULL) + 36000;

    Kentec320x240x16_SSD2119Init((uint32_t)cpuFreq.lo);
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);
    System_printf("Screen Initialized\n");
    System_flush();

    TouchScreenInit((uint32_t)cpuFreq.lo);
    TouchScreenCallbackSet(WidgetPointerMessage);
    System_printf("Touch Initialized\n");
    System_flush();

    g_ui32Panel = 0;
    // Add previous and next buttons to the widget tree.
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sStartStop);
    // Add the first panel to the widget tree.
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);
    // Issue the initial paint request to the widgets.
    WidgetPaint(WIDGET_ROOT);

    // Add the test button to the second panel
    WidgetAdd((tWidget *)&g_sPanel2, (tWidget *) &g_sChooseMotor);
    WidgetAdd((tWidget *)&g_sPanel2, (tWidget *) &g_sChooseLight);
    WidgetAdd((tWidget *)&g_sPanel2, (tWidget *) &g_sChooseTemp);
    WidgetAdd((tWidget *)&g_sPanel2, (tWidget *) &g_sChooseAcc);

    while(1) {
        if (day && lux < 5) {
            sRect.i16XMin = 260;
            sRect.i16YMin = 1;
            sRect.i16XMax = GrContextDpyWidthGet(&sContext)-2;
            sRect.i16YMax = 22;
            GrContextForegroundSet(&sContext, ClrDarkBlue);
            GrRectFill(&sContext, &sRect);
            GrContextForegroundSet(&sContext, ClrWhite);
            GrContextFontSet(&sContext, &g_sFontCm20);
            GrStringDraw(&sContext, "Night", -1,
                         GrContextDpyWidthGet(&sContext)-2-GrStringWidthGet(&sContext, "Night", sizeof("Night")),
                         2, 0);
            GPIO_write(Board_LED1, 1);
            day = false;
        }
        else if (!day && lux > 5) {
            sRect.i16XMin = 260;
            sRect.i16YMin = 1;
            sRect.i16XMax = GrContextDpyWidthGet(&sContext)-2;
            sRect.i16YMax = 22;
            GrContextForegroundSet(&sContext, ClrDarkBlue);
            GrRectFill(&sContext, &sRect);
            GrContextForegroundSet(&sContext, ClrWhite);
            GrContextFontSet(&sContext, &g_sFontCm20);
            GrStringDraw(&sContext, "Day", -1,
                         GrContextDpyWidthGet(&sContext)-2-GrStringWidthGet(&sContext, "Day", sizeof("Day")),
                         2, 0);
            GPIO_write(Board_LED1, 0);
            day = true;
        }
        if (updateTime) {
            Semaphore_pend(semTime, BIOS_WAIT_FOREVER);
                updateTime = false;
            Semaphore_post(semTime);
            t++;
            tm = gmtime(&t);

            sRect.i16XMin = 1;
            sRect.i16YMin = 1;
            sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 100;
            sRect.i16YMax = 22;
            GrContextForegroundSet(&sContext, ClrDarkBlue);
            GrRectFill(&sContext, &sRect);

            GrContextForegroundSet(&sContext, ClrWhite);
            GrContextFontSet(&sContext, &g_sFontCm20);
            IntMasterDisable();
            strftime(tempStr, sizeof(tempStr), "%d-%m-%Y %H:%M:%S", tm);
            IntMasterEnable();
            GrStringDraw(&sContext, tempStr, -1, 3, 2, 0);

            if (display_page == 2) {
                // draw rectangle around previous RPM
                sRect_rpm.i16XMin = 35;
                sRect_rpm.i16YMin = 70-2-18;
                sRect_rpm.i16XMax = 215;
                sRect_rpm.i16YMax = 70;
                GrContextForegroundSet(&sContext, ClrBlack);
                GrRectFill(&sContext, &sRect_rpm);
                // draw current RPM
                GrContextFontSet(&sContext, &g_sFontCm18);
                GrContextForegroundSet(&sContext, ClrWhite);
                sprintf(tempStr, "RPM: %d", rpm);
                GrStringDraw(&sContext, tempStr, -1, 35, 70-2-18, 0);
                // draw rectangle around previous graph
                sRect_rpm.i16XMin = 215-29*6;
                sRect_rpm.i16YMin = 70;
                sRect_rpm.i16XMax = 215;
                sRect_rpm.i16YMax = 170;
                GrContextForegroundSet(&sContext, ClrDarkBlue);
                GrRectFill(&sContext, &sRect_rpm);
                // draw graph
                int i; int x = 215-29*6;
                GrContextForegroundSet(&sContext, ClrWhite);
                for (i = 0; i < 29; i++) {
                    GrLineDraw(&sContext, x,   170-(float)motor_rpm[i]/7500*(170-70),
                                          x+6, 170-(float)motor_rpm[i+1]/7500*(170-70));
                    x += 6;
                }
                // label graph
                GrStringDraw(&sContext, "7500", -1, 35-GrStringWidthGet(&sContext, "7500", sizeof("7500")), 70+2, 0);
                GrStringDraw(&sContext, "0", -1, 35-GrStringWidthGet(&sContext, "0", sizeof("0")), 170-18-2, 0);
                GrStringDraw(&sContext, "3 seconds ago", -1, 35, 172, 0);
                GrStringDraw(&sContext, "Now", -1,
                             215-2-GrStringWidthGet(&sContext, "Now", sizeof("Now")),
                             172, 0);
            }
        }
        // Process any messages in the widget message queue.
        WidgetMessageQueueProcess();
        Task_sleep(20);
    }
}
