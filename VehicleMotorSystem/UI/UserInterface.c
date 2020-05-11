#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "images.h"

//*****************************************************************************
//
// Forward declarations for the globals required to define the widgets at
// compile-time.
//
//*****************************************************************************
void OnPrevious(tWidget *psWidget);
void OnNext(tWidget *psWidget);
void OnPanel1(tWidget *psWidget, tContext *psContext);
void OnPanel2(tWidget *psWidget, tContext *psContext);
extern tCanvasWidget g_psPanels[];
extern UART_Handle uart;
Types_FreqHz cpuFreq;
tContext sContext;
tRectangle sRect;

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

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, &g_sFontCm20);
    GrStringDrawCentered(&sContext, "Test1", -1,
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
// The number of panels.
//
//*****************************************************************************
#define NUM_PANELS 2

//*****************************************************************************
//
// The buttons at the bottom of the screen.
//
//*****************************************************************************
RectangularButton(g_sPrevious, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 0, 200,
                  50, 40, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  &g_sFontCm20, "-", g_pui8Blue50x50, g_pui8Blue50x50Press, 0, 0,
                  OnPrevious);

RectangularButton(g_sNext, 0, 0, 0, &g_sKentec320x240x16_SSD2119, 270, 200,
                  50, 40, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, &g_sFontCm20, "+", g_pui8Blue50x50,
                  g_pui8Blue50x50Press, 0, 0, OnNext);

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
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Clear the previous button from the display since the first panel is
    // being displayed.
    //
    PushButtonImageOff(&g_sPrevious);
    PushButtonTextOff(&g_sPrevious);
    PushButtonFillOn(&g_sPrevious);
    WidgetPaint((tWidget *)&g_sPrevious);

    //
    // Display the next button.
    //
    PushButtonImageOn(&g_sNext);
    PushButtonTextOn(&g_sNext);
    PushButtonFillOff(&g_sNext);
    WidgetPaint((tWidget *)&g_sNext);

    sRect.i16XMin = 1;
    sRect.i16YMin = 1;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 2;
    sRect.i16YMax = 22;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextFontSet(&sContext, &g_sFontCm20);
    GrStringDrawCentered(&sContext, "Panel 1", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 8, 0);
}

//*****************************************************************************
//
// Handles presses of the next panel button.
//
//*****************************************************************************
void
OnNext(tWidget *psWidget) {

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ui32Panel));
    WidgetPaint((tWidget *)(g_psPanels + g_ui32Panel));

    //
    // Display the previous button.
    //
    PushButtonImageOn(&g_sPrevious);
    PushButtonTextOn(&g_sPrevious);
    PushButtonFillOff(&g_sPrevious);
    WidgetPaint((tWidget *)&g_sPrevious);

    //
    // Clear the next button from the display since the last panel is being
    // displayed.
    //
    PushButtonImageOff(&g_sNext);
    PushButtonTextOff(&g_sNext);
    PushButtonFillOn(&g_sNext);
    WidgetPaint((tWidget *)&g_sNext);

    sRect.i16XMin = 1;
    sRect.i16YMin = 1;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 2;
    sRect.i16YMax = 22;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextFontSet(&sContext, &g_sFontCm20);
    GrStringDrawCentered(&sContext, "Panel 2", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 8, 0);
}

//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void
OnPanel1(tWidget *psWidget, tContext *psContext) {

}

//*****************************************************************************
//
// Handles paint requests for the primitives canvas widget.
//
//*****************************************************************************
void
OnPanel2(tWidget *psWidget, tContext *psContext) {

}



Void UiStart() {

    BIOS_getCpuFreq(&cpuFreq);
    initScreen();
    initTouch();
    System_printf("Starting UI\n");
    System_flush();
    char tempStr[40];
    sprintf(tempStr, "\n\n\n\n Starting UI\n");
    UART_write(uart, tempStr, strlen(tempStr));

    //
    // Add previous and next buttons to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);

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

        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}
