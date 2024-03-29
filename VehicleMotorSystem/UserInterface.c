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

#define GRAPH_MOTOR 1
#define GRAPH_LIGHT 2
#define GRAPH_TEMP  3
#define GRAPH_ACCEL 4

#define TEMP_MAX    80
#define LUX_MAX     250
#define ACCEL_MAX   40
#define CURR_MAX    8000
#define WATT_MAX    30

#define GRAPH_RIGHT_EDGE    215
#define GRAPH_TOP_EDGE      70
#define GRAPH_BOTTOM_EDGE   170
#define GRAPH_NUM_POINTS    30
#define GRAPH_POINTS_WIDTH  6
#define FONT_SIZE           18

//*****************************************************************************
// Shared resources that use semaphores
//*****************************************************************************
time_t t; // semTime
struct tm *tm;

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
                   105, 35, 190, 30, 0, 100, 100,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "7200", 0, 0, OnSliderChange ),
    SliderStruct(  g_psPanels, g_psSliders + 2, 0, &g_sKentec320x240x16_SSD2119,
                   105, 77, 190, 30, 0, 100, 100,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "40", 0, 0, OnSliderChange ),
    SliderStruct(  g_psPanels, g_psSliders + 3, 0, &g_sKentec320x240x16_SSD2119,
                   105, 119, 190, 30, 0, 100, 100,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "8000", 0, 0, OnSliderChange),
    SliderStruct(  g_psPanels, &g_sPanel1, 0, &g_sKentec320x240x16_SSD2119,
                   105, 161, 190, 30, 0, 100, 100,
                   (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE | SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                   ClrDarkOrange, ClrBlack, ClrSilver, ClrWhite, ClrWhite, &g_sFontCm20, "80", 0, 0, OnSliderChange),
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

int graph_param = GRAPH_MOTOR;
void chooseMotor(tWidget *psWidget) {
    graph_param = GRAPH_MOTOR;
}

void chooseLight(tWidget *psWidget) {
    graph_param = GRAPH_LIGHT;
}

void chooseTemp(tWidget *psWidget) {
    graph_param = GRAPH_TEMP;
}

void chooseAcc(tWidget *psWidget) {
    graph_param = GRAPH_ACCEL;
}

RectangularButton(g_sChooseMotor, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 40, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Motor", 0, 0, 0, 0, chooseMotor);

RectangularButton(g_sChooseLight, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 80, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Light", 0, 0, 0, 0, chooseLight);

RectangularButton(g_sChooseTemp, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 120, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Temp.", 0, 0, 0, 0, chooseTemp);

RectangularButton(g_sChooseAcc, &g_sPanel2, 0, 0, &g_sKentec320x240x16_SSD2119, 230, 160, 80, 35,
                  (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT | PB_STYLE_AUTO_REPEAT),
                  ClrDarkBlue, ClrBlue, ClrWhite, ClrWhite, &g_sFontCm20,
                  "Acc.", 0, 0, 0, 0, chooseAcc);

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

volatile int rpm_screen = RPM_MAX;
int AccelerationLimit = ACCEL_MAX; // semAccelerationLimit
int CurrentLimit = CURR_MAX; // semCurrentLimit
int TempLimit = TEMP_MAX; // semTempLimit
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
            rpm_screen = (int)round(RPM_MAX * ((float)i32Value / 100));
        Semaphore_post(semDutyScreen);

        usprintf(pcSpeedText, "%3d", rpm_screen);
        SliderTextSet(&g_psSliders[MOTOR_SPEED_SLIDER], pcSpeedText);
        WidgetPaint((tWidget *)&g_psSliders[MOTOR_SPEED_SLIDER]);
    }
    else if(psWidget == (tWidget *)&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER]) {
        AccelerationLimit = (int)(ACCEL_MAX * (float)i32Value / 100);

        usprintf(pcAccelText, "%3d", AccelerationLimit);
        SliderTextSet(&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER], pcAccelText);
        WidgetPaint((tWidget *)&g_psSliders[ALLOWABLE_ACCELLERATION_SLIDER]);
    }
    else if(psWidget == (tWidget *)&g_psSliders[CURRENT_LIMIT_SLIDER]) {
        CurrentLimit = (int)(CURR_MAX * (float)i32Value / 100);

        usprintf(pcAmpereText, "%3d", CurrentLimit);
        SliderTextSet(&g_psSliders[CURRENT_LIMIT_SLIDER], pcAmpereText);
        WidgetPaint((tWidget *)&g_psSliders[CURRENT_LIMIT_SLIDER]);
    }
    else if(psWidget == (tWidget *)&g_psSliders[TEMPERATURE_LIMIT_SLIDER]) {
        TempLimit = (int)(TEMP_MAX * (float)i32Value / 100);

        usprintf(pcTempText, "%3d", TempLimit);
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
bool updatePlot;
void UpdateTime() {
    int i = 0;
    while(1) {
        if (i % 10 == 0) {
            Semaphore_pend(semTime, BIOS_WAIT_FOREVER);
                updateTime = true;
            Semaphore_post(semTime);
            i = 0;
        }
        if (i % 5 == 0) {
            Semaphore_pend(semPlot, BIOS_WAIT_FOREVER);
                updatePlot = true;
            Semaphore_post(semPlot);
        }
        i++;
        Task_sleep(100);
    }
}

extern int motor_rpm[];
extern int desired_motor_rpm[];
extern float motorPowerArray[];
extern int luxArray[];
extern float boardTempArray[];
extern float motorTempArray[];
extern float accelArray[];
//*****************************************************************************
// Main display functionality.
//*****************************************************************************
tContext sContext;
void UiStart() {
    tRectangle sRect;
    tRectangle sRect_graph;
    char tempStr[40];
    char tempLabel[40];
    float tempData1[30] = {0};
    float tempData2[30] = {0};
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
    Semaphore_post(semScreenInit);

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
        if (day && luxArray[29] < 5) {
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
        else if (!day && luxArray[29] > 5) {
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
        }
        if (updatePlot && display_page == 2) {
            Semaphore_pend(semPlot, BIOS_WAIT_FOREVER);
                updatePlot = false;
            Semaphore_post(semPlot);
            int graph_left_edge = GRAPH_RIGHT_EDGE - (GRAPH_NUM_POINTS-1) * GRAPH_POINTS_WIDTH;
            int graph_height = GRAPH_BOTTOM_EDGE - GRAPH_TOP_EDGE;
            int i; int x = graph_left_edge;
            // clear the left labels
            sRect_graph.i16XMin = 1;
            sRect_graph.i16YMin = GRAPH_TOP_EDGE;
            sRect_graph.i16XMax = graph_left_edge;
            sRect_graph.i16YMax = GRAPH_BOTTOM_EDGE;
            GrContextForegroundSet(&sContext, ClrBlack);
            GrRectFill(&sContext, &sRect_graph);
            // clear the top labels
            sRect_graph.i16XMin = graph_left_edge;
            sRect_graph.i16YMin = GRAPH_TOP_EDGE - 2*(2+FONT_SIZE);
            sRect_graph.i16XMax = GRAPH_RIGHT_EDGE;
            sRect_graph.i16YMax = GRAPH_TOP_EDGE;
            GrContextForegroundSet(&sContext, ClrBlack);
            GrRectFill(&sContext, &sRect_graph);
            // plot graph background
            sRect_graph.i16XMin = graph_left_edge;
            sRect_graph.i16YMin = GRAPH_TOP_EDGE;
            sRect_graph.i16XMax = GRAPH_RIGHT_EDGE;
            sRect_graph.i16YMax = GRAPH_BOTTOM_EDGE;
            GrContextForegroundSet(&sContext, ClrDarkBlue);
            GrRectFill(&sContext, &sRect_graph);
            // define the plotting variables
            GrContextFontSet(&sContext, &g_sFontCm18);

            switch (graph_param) {
                case GRAPH_MOTOR:
                    Semaphore_pend(semRPM, BIOS_WAIT_FOREVER);
                        for (i = 0; i < GRAPH_NUM_POINTS; i++) {
                            tempData1[i] = motor_rpm[i];
                            tempData2[i] = motorPowerArray[i];
                        }
                    Semaphore_post(semRPM);
                    // print the motor RPM
                    GrContextForegroundSet(&sContext, ClrRed);
                    sprintf(tempStr, "Motor RPM: %d", (int)tempData1[(GRAPH_NUM_POINTS-1)]);
                    GrStringDraw(&sContext, tempStr, -1, graph_left_edge, GRAPH_TOP_EDGE-2*(2+FONT_SIZE), 0);
                    // plot the graph
                    for (i = 0; i < (GRAPH_NUM_POINTS-1); i++) {
                        GrLineDraw(&sContext,
                                   x, GRAPH_BOTTOM_EDGE - (float)tempData1[i] / RPM_MAX * graph_height,
                                   x+GRAPH_POINTS_WIDTH, GRAPH_BOTTOM_EDGE - (float)tempData1[i+1] / RPM_MAX * graph_height);
                        x += GRAPH_POINTS_WIDTH;
                    }
                    sprintf(tempLabel, "%d", RPM_MAX);
                    GrStringDraw(&sContext, tempLabel, -1, GRAPH_RIGHT_EDGE-GrStringWidthGet(&sContext, tempLabel, sizeof(tempLabel)), GRAPH_TOP_EDGE+2, 0);
                    GrStringDraw(&sContext, "0", -1, GRAPH_RIGHT_EDGE-GrStringWidthGet(&sContext, "0", sizeof("0")), GRAPH_BOTTOM_EDGE-2-FONT_SIZE, 0);
                    // print the estimated power
                    GrContextForegroundSet(&sContext, ClrWhite);
                    sprintf(tempStr, "Est. Power: %d", (int)tempData2[(GRAPH_NUM_POINTS-1)]);
                    GrStringDraw(&sContext, tempStr, -1, graph_left_edge, GRAPH_TOP_EDGE-2-FONT_SIZE, 0);
                    x = graph_left_edge;
                    for (i = 0; i < (GRAPH_NUM_POINTS-1); i++) {
                        GrLineDraw(&sContext,
                                   x, GRAPH_BOTTOM_EDGE - tempData2[i] / WATT_MAX * graph_height,
                                   x+GRAPH_POINTS_WIDTH, GRAPH_BOTTOM_EDGE - tempData2[i+1] / WATT_MAX * graph_height);
                        x += GRAPH_POINTS_WIDTH;
                    }
                    sprintf(tempLabel, "%d", WATT_MAX);
                    break;

                case GRAPH_LIGHT:
                    Semaphore_pend(semLUX, BIOS_WAIT_FOREVER);
                        for (i = 0; i < GRAPH_NUM_POINTS; i++) {
                            tempData1[i] = luxArray[i];
                        }
                    Semaphore_post(semLUX);
                    // print the current value to the screen
                    GrContextForegroundSet(&sContext, ClrWhite);
                    sprintf(tempStr, "LUX: %d", (int)tempData1[(GRAPH_NUM_POINTS-1)]);
                    GrStringDraw(&sContext, tempStr, -1, graph_left_edge, GRAPH_TOP_EDGE-2-FONT_SIZE, 0);
                    // plot the graph
                    for (i = 0; i < (GRAPH_NUM_POINTS-1); i++) {
                        GrLineDraw(&sContext,
                                   x, GRAPH_BOTTOM_EDGE - (float)tempData1[i] / LUX_MAX * graph_height,
                                   x+GRAPH_POINTS_WIDTH, GRAPH_BOTTOM_EDGE - (float)tempData1[i+1] / LUX_MAX * graph_height);
                        x += GRAPH_POINTS_WIDTH;
                    }
                    sprintf(tempLabel, "%d", LUX_MAX);
                    break;

                case GRAPH_TEMP:
                    Semaphore_pend(semTEMP, BIOS_WAIT_FOREVER);
                        for (i = 0; i < GRAPH_NUM_POINTS; i++) {
                            tempData1[i] = motorTempArray[i];
                            tempData2[i] = boardTempArray[i];
                        }
                    Semaphore_post(semTEMP);
                    // print the ambient temp data
                    GrContextForegroundSet(&sContext, ClrRed);
                    sprintf(tempStr, "Ambient Temp: %d", (int)tempData1[(GRAPH_NUM_POINTS-1)]);
                    GrStringDraw(&sContext, tempStr, -1, graph_left_edge, GRAPH_TOP_EDGE-2*(2+FONT_SIZE), 0);
                    for (i = 0; i < (GRAPH_NUM_POINTS-1); i++) {
                        GrLineDraw(&sContext,
                                   x, GRAPH_BOTTOM_EDGE - tempData1[i] / TEMP_MAX * graph_height,
                                   x+GRAPH_POINTS_WIDTH, GRAPH_BOTTOM_EDGE - tempData1[i+1] / TEMP_MAX * graph_height);
                        x += GRAPH_POINTS_WIDTH;
                    }
                    // print the motor temp data
                    GrContextForegroundSet(&sContext, ClrWhite);
                    sprintf(tempStr, "Motor Temp: %d", (int)tempData2[(GRAPH_NUM_POINTS-1)]);
                    GrStringDraw(&sContext, tempStr, -1, graph_left_edge, GRAPH_TOP_EDGE-2-FONT_SIZE, 0);
                    x = graph_left_edge;
                    for (i = 0; i < (GRAPH_NUM_POINTS-1); i++) {
                        GrLineDraw(&sContext,
                                   x, GRAPH_BOTTOM_EDGE - tempData2[i] / TEMP_MAX * graph_height,
                                   x+GRAPH_POINTS_WIDTH, GRAPH_BOTTOM_EDGE - tempData2[i+1] / TEMP_MAX * graph_height);
                        x += GRAPH_POINTS_WIDTH;
                    }
                    sprintf(tempLabel, "%d", TEMP_MAX);
                    break;

                case GRAPH_ACCEL:
                    Semaphore_pend(semACCEL, BIOS_WAIT_FOREVER);
                        for (i = 0; i < GRAPH_NUM_POINTS; i++) {
                            tempData1[i] = accelArray[i];
                        }
                    Semaphore_post(semACCEL);
                    // print the current value to the screen
                    GrContextForegroundSet(&sContext, ClrWhite);
                    sprintf(tempStr, "Accelerometer: %d", (int)tempData1[(GRAPH_NUM_POINTS-1)]);
                    GrStringDraw(&sContext, tempStr, -1, graph_left_edge, GRAPH_TOP_EDGE-2-FONT_SIZE, 0);
                    // plot the graph
                    for (i = 0; i < (GRAPH_NUM_POINTS-1); i++) {
                        GrLineDraw(&sContext,
                                   x, GRAPH_BOTTOM_EDGE - (float)tempData1[i] / ACCEL_MAX * graph_height,
                                   x+GRAPH_POINTS_WIDTH, GRAPH_BOTTOM_EDGE - (float)tempData1[i+1] / ACCEL_MAX * graph_height);
                        x += GRAPH_POINTS_WIDTH;
                    }
                    sprintf(tempLabel, "%d", ACCEL_MAX);
                    break;

                default:
                    break;
            }

            // label the graph
            GrContextForegroundSet(&sContext, ClrWhite);
            GrStringDraw(&sContext, tempLabel, -1, 35-GrStringWidthGet(&sContext, tempLabel, sizeof(tempLabel)), GRAPH_TOP_EDGE+2, 0);
            GrStringDraw(&sContext, "0", -1, 35-GrStringWidthGet(&sContext, "0", sizeof("0")), GRAPH_BOTTOM_EDGE-2-FONT_SIZE, 0);
            GrStringDraw(&sContext, "3 seconds ago", -1, 35, GRAPH_BOTTOM_EDGE+2, 0);
            GrStringDraw(&sContext, "Now", -1,
                         GRAPH_RIGHT_EDGE-2-GrStringWidthGet(&sContext, "Now", sizeof("Now")),
                         GRAPH_BOTTOM_EDGE+2, 0);
        }
        // Process any messages in the widget message queue.
        WidgetMessageQueueProcess();
        Task_sleep(20);
    }
}
