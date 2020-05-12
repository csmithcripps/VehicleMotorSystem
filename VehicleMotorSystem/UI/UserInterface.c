#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "utils/ustdlib.h"
#include "utils/globaldefines.h"
#include "drivers/Kentec320x240x16_ssd2119_spi.h"
#include "drivers/touch.h"
#include "Board.h"


//*****************************************************************************
//
// Forward declarations for the globals required to define the widgets at
// compile-time.
//
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
//
// Shared resources that use semaphores
//
//*****************************************************************************
time_t t; // semTime
extern int SpeedLimit; // semSpeedLimit
extern int AccelerationLimit; // semAccelerationLimit
extern int CurrentLimit; // semCurrentLimit
extern int TempLimit; // semTempLimit


//*****************************************************************************
//
// The first panel
//
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
Canvas(g_sSliderValueCanvas, g_psPanels, &g_sSliderValueCanvas1, 0,
       &g_sKentec320x240x16_SSD2119, 0, 0, 0, 0,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       &g_sFontCm18, "", 0, 0);

tSliderWidget g_psSliders[] =
{
 SliderStruct(g_psPanels, g_psSliders + 1, 0,
              &g_sKentec320x240x16_SSD2119, 98, 35, 220, 30, 0, 100, 50,
              (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                      SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                      ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                      &g_sFontCm20, "50%", 0, 0, OnSliderChange),
                      SliderStruct(g_psPanels, g_psSliders + 2, 0,
                                   &g_sKentec320x240x16_SSD2119, 98, 77, 220, 30, 0, 100, 50,
                                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                                           SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                                           ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                                           &g_sFontCm20, "50%", 0, 0, OnSliderChange),
                                           SliderStruct(g_psPanels, g_psSliders + 3, 0,
                                                        &g_sKentec320x240x16_SSD2119, 98, 119, 220, 30, 0, 100, 50,
                                                        (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                                                                SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                                                                ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                                                                &g_sFontCm20, "50%", 0, 0, OnSliderChange),
                                                                SliderStruct(g_psPanels, &g_sSliderValueCanvas, 0,
                                                                             &g_sKentec320x240x16_SSD2119, 98, 161, 220, 30, 0, 100, 50,
                                                                             (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                                                                                     SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                                                                                     ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                                                                                     &g_sFontCm20, "50%", 0, 0, OnSliderChange),
};

#define MOTOR_SPEED_SLIDER   0
#define ALLOWABLE_ACCELLERATION_SLIDER   1
#define CURRENT_LIMIT_SLIDER   2
#define TEMPERATURE_LIMIT_SLIDER   3

//*****************************************************************************
//
// The second panel
//
//*****************************************************************************
Canvas(g_sPanel2, g_psPanels + 1, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnPanel2);

//*****************************************************************************
//
// An array of canvas widgets, one per panel. Each canvas is filled with
// black, overwriting the contents of the previous panel.
//
//*****************************************************************************
tCanvasWidget g_psPanels[] =
{
 CanvasStruct(0, 0, &g_psSliders, &g_sKentec320x240x16_SSD2119, 0, 24,
              320, 176, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),

              CanvasStruct(0, 0, &g_sPanel2, &g_sKentec320x240x16_SSD2119, 0, 24,
                           320, 176, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

//*****************************************************************************
//
// The buttons at the bottom of the screen.
//
//*****************************************************************************
RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 200, 50, 40,
                  (PB_STYLE_FILL | PB_STYLE_AUTO_REPEAT), ClrBlack, ClrBlack, ClrWhite,
                  ClrWhite, &g_sFontCm20, "-", 0, 0, 0, 0, OnPrevious);

RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 200, 50, 40,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20, "+", 0, 0, 0, 0, OnNext);

RectangularButton(g_sStartStop, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 70, 200, 180, 40,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrLimeGreen, ClrGreen, ClrWhite, ClrWhite, &g_sFontCm20b, "Start", 0, 0, 0, 0, OnStartStop);

//*****************************************************************************
//
// The panel that is currently being displayed.
//
//*****************************************************************************
uint32_t g_ui32Panel;

//*****************************************************************************
//
// Handles presses of the previous panel button.
//
//*****************************************************************************
void
OnPrevious(tWidget *psWidget) {

    //
    // There is nothing to be done if the first panel is already being
    // displayed.
    //
    if(g_ui32Panel == 0) {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Decrement the panel index.
    //
    g_ui32Panel--;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // See if this is the first panel.
    //
    if(g_ui32Panel == 0) {
        //
        // Clear the previous button from the display since the first panel is
        // being displayed.
        //
        PushButtonTextOff(&g_sPrevious);
        PushButtonOutlineOff(&g_sPrevious);
        PushButtonFillColorSet(&g_sPrevious, ClrBlack);
        PushButtonFillColorPressedSet(&g_sPrevious, ClrBlack);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if the previous panel was the last panel.
    //
    if(g_ui32Panel == 0) {
        //
        // Display the next button.
        //
        PushButtonTextOn(&g_sNext);
        PushButtonOutlineOn(&g_sNext);
        PushButtonFillColorSet(&g_sNext, ClrDarkBlue);
        PushButtonFillColorPressedSet(&g_sNext, ClrBlue);
        WidgetPaint((tWidget *)&g_sNext);
    }
}

//*****************************************************************************
//
// Handles presses of the next panel button.
//
//*****************************************************************************
void OnNext(tWidget *psWidget) {

    //
    // There is nothing to be done if the last panel is already being
    // displayed.
    //
    if(g_ui32Panel == 1) {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Increment the panel index.
    //
    g_ui32Panel++;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // See if the previous panel was the first panel.
    //
    if(g_ui32Panel == 1) {
        //
        // Display the previous button.
        //
        PushButtonTextOn(&g_sPrevious);
        PushButtonOutlineOn(&g_sPrevious);
        PushButtonFillColorSet(&g_sPrevious, ClrDarkBlue);
        PushButtonFillColorPressedSet(&g_sPrevious, ClrBlue);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if this is the last panel.
    //
    if(g_ui32Panel == 1) {
        //
        // Clear the next button from the display since the last panel is being
        // displayed.
        //
        PushButtonTextOff(&g_sNext);
        PushButtonOutlineOff(&g_sNext);
        PushButtonFillColorSet(&g_sNext, ClrBlack);
        PushButtonFillColorPressedSet(&g_sNext, ClrBlack);
        WidgetPaint((tWidget *)&g_sNext);
    }
}

//*****************************************************************************
//
// Handles presses of the start/stop button.
//
//*****************************************************************************
bool MotorOn = false;
void OnStartStop(tWidget *psWidget) {
    if (MotorOn) {
        Stop();
    }
    else {
        Start();
    }
}


void OnSliderChange(tWidget *psWidget, int32_t i32Value) {
    static char pcSpeedText[5];
    static char pcAccelText[5];
    static char pcAmpereText[5];
    static char pcTempText[5];

    if(psWidget == (tWidget *)&g_psSliders[MOTOR_SPEED_SLIDER])
    {
        Semaphore_pend(semSpeedLimit, BIOS_WAIT_FOREVER);
        SpeedLimit = MAX_SPEED * i32Value;
        Semaphore_post(semSpeedLimit);

        usprintf(pcSpeedText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[MOTOR_SPEED_SLIDER], pcSpeedText);
        WidgetPaint((tWidget *)&g_psSliders[MOTOR_SPEED_SLIDER]);
    }

    else if(psWidget == (tWidget *)&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER])
    {
        Semaphore_pend(semSpeedLimit, BIOS_WAIT_FOREVER);
        SpeedLimit = MAX_ACCELERATION * i32Value;
        Semaphore_post(semSpeedLimit);

        usprintf(pcAccelText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER], pcAccelText);
        WidgetPaint((tWidget *)&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER]);
    }

    else if(psWidget == (tWidget *)&g_psSliders[CURRENT_LIMIT_SLIDER])
    {
        Semaphore_pend(semSpeedLimit, BIOS_WAIT_FOREVER);
        SpeedLimit = MAX_CURRENT * i32Value;
        Semaphore_post(semSpeedLimit);

        usprintf(pcAmpereText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[CURRENT_LIMIT_SLIDER], pcAmpereText);
        WidgetPaint((tWidget *)&g_psSliders[CURRENT_LIMIT_SLIDER]);
    }

    else if(psWidget == (tWidget *)&g_psSliders[TEMPERATURE_LIMIT_SLIDER])
    {
        Semaphore_pend(semSpeedLimit, BIOS_WAIT_FOREVER);
        SpeedLimit = MAX_TEMPARATURE * i32Value;
        Semaphore_post(semSpeedLimit);

        usprintf(pcTempText, "%3d%%", i32Value);
        SliderTextSet(&g_psSliders[TEMPERATURE_LIMIT_SLIDER], pcTempText);
        WidgetPaint((tWidget *)&g_psSliders[TEMPERATURE_LIMIT_SLIDER]);
    }
}

//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void OnPanel1(tWidget *psWidget, tContext *psContext) {
    GrContextFontSet(psContext, &g_sFontCm18);
    GrContextForegroundSet(psContext, ClrWhite);
    GrStringDraw(psContext, "Motor Speed", -1,
                 5, 40, 0);
}

//*****************************************************************************
//
// Handles paint requests for the primitives canvas widget.
//
//*****************************************************************************
void OnPanel2(tWidget *psWidget, tContext *psContext) {

}

void Start() {
    MotorOn = true;
    GPIO_write(Board_LED0, Board_LED_ON);
    PushButtonTextSet(&g_sStartStop, "Stop");
    PushButtonFillColorSet(&g_sStartStop, ClrRed);
    PushButtonFillColorPressedSet(&g_sStartStop, ClrDarkRed);
    WidgetPaint((tWidget *)&g_sStartStop);
}

void Stop() {
    MotorOn = false;
    GPIO_write(Board_LED0, Board_LED_OFF);
    PushButtonTextSet(&g_sStartStop, "Start");
    PushButtonFillColorSet(&g_sStartStop, ClrLimeGreen);
    PushButtonFillColorPressedSet(&g_sStartStop, ClrGreen);
    WidgetPaint((tWidget *)&g_sStartStop);
}


bool update;
void UpdateTime() {
    while(1) {
        if (t != time(NULL) + 36000) {
            Semaphore_pend(semTime, BIOS_WAIT_FOREVER);
            update = true;
            Semaphore_post(semTime);
            t = time(NULL) + 36000;
        }
        Task_sleep(100);
    }
}

void UiStart() {

    tContext sContext;
    tRectangle sRect;
    char tempStr[40];
    Types_FreqHz cpuFreq;
    BIOS_getCpuFreq(&cpuFreq);

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

    //
    // Add previous and next buttons to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sStartStop);

    //
    // Add the first panel to the widget tree.
    //
    g_ui32Panel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    while(1) {

        if (update) {

            Semaphore_pend(semTime, BIOS_WAIT_FOREVER);
            update = false;
            Semaphore_post(semTime);

            struct tm *tm = gmtime(&t);

            sRect.i16XMin = 1;
            sRect.i16YMin = 1;
            sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 2;
            sRect.i16YMax = 22;
            GrContextForegroundSet(&sContext, ClrDarkBlue);
            GrRectFill(&sContext, &sRect);

            GrContextForegroundSet(&sContext, ClrWhite);
            GrContextFontSet(&sContext, &g_sFontCm20);
            strftime(tempStr, sizeof(tempStr), "%d-%m-%Y %H:%M:%S", tm);
            GrStringDraw(&sContext, tempStr, -1,
                         3, 2, 0);

        }
        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();

        Task_sleep(1);
    }
}
