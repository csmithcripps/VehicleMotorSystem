#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>
#include <ti/sysbios/BIOS.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/sysbios/knl/Task.h>
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"
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
void OnPanel1(tWidget *psWidget, tContext *psContext);
void OnPanel2(tWidget *psWidget, tContext *psContext);
extern tCanvasWidget g_psPanels[];
extern UART_Handle uart;

Types_FreqHz cpuFreq;
tContext sContext;
tRectangle sRect;
uint32_t g_ui32Panel;
time_t t;
struct tm *tm;
char tempStr[40];
bool MotorOn = false;

//*****************************************************************************
//
// Initialization of the screen
//
//*****************************************************************************
void initScreen() {
    Kentec320x240x16_SSD2119Init((uint32_t)cpuFreq.lo);
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);

    //
    // Fill the top 24 rows of the screen with red to create the banner.
    //
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 23;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    t = time(NULL) + 36000;
    tm = gmtime(&t);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, &g_sFontCm20);
    sprintf(tempStr, "%02d/%02d/%02d %02d:%02d:%02d", tm->tm_mday, tm->tm_mon+1, tm->tm_year-100, tm->tm_hour, tm->tm_min, tm->tm_sec);
    GrStringDrawCentered(&sContext, tempStr, -1,
                         GrContextDpyWidthGet(&sContext) / 2, 8, 0);

    System_printf("Screen Initialized\n");
    System_flush();
}

//*****************************************************************************
//
// Initialization of the touch input
//
//*****************************************************************************
void initTouch() {
    TouchScreenInit((uint32_t)cpuFreq.lo);
    TouchScreenCallbackSet(WidgetPointerMessage);
    System_printf("Touch Initialized\n");
    System_flush();
}

//*****************************************************************************
//
// The first panel
//
//*****************************************************************************
Canvas(g_sPanel1, g_psPanels, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 24,
       320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnPanel1);

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
 CanvasStruct(0, 0, &g_sPanel1, &g_sKentec320x240x16_SSD2119, 0, 24,
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
// Handles presses of the previous panel button.
//
//*****************************************************************************
void
OnPrevious(tWidget *psWidget) {

    //
    // There is nothing to be done if the first panel is already being
    // displayed.
    //
    if(g_ui32Panel == 0)
    {
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
    if(g_ui32Panel == 0)
    {
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
    if(g_ui32Panel == 0)
    {
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
    if(g_ui32Panel == 1)
    {
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
    if(g_ui32Panel == 1)
    {
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
void OnStartStop(tWidget *psWidget) {
    if (MotorOn) {
        Stop();
    }
    else {
        Start();
    }
}

//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void OnPanel1(tWidget *psWidget, tContext *psContext) {

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

void UpdateTime() {

}

void UiStart() {

    BIOS_getCpuFreq(&cpuFreq);
    initScreen();
    initTouch();
    System_printf("Starting UI\n");
    System_flush();;
    sprintf(tempStr, "\n\n\n\n Starting UI\n");
    UART_write(uart, tempStr, strlen(tempStr));

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

    while (1) {
        //        if (t != time(NULL + 36000) {
        //            UpdateTime();
        //        }

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}
