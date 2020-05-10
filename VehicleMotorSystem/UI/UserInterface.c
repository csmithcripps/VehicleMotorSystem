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

extern UART_Handle uart;

Types_FreqHz cpuFreq;
tContext sContext;
tRectangle sRect;

void initScreen() {
    System_printf("initializing screen\n");
    System_flush();
    Kentec320x240x16_SSD2119Init((uint32_t)cpuFreq.lo);
    GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = 319;
    sRect.i16YMax = 239;
    GrContextForegroundSet(&sContext, ClrBlue);
    GrRectFill(&sContext, &sRect);

}

void initTouch() {
    System_printf("initializing touch\n");
    System_flush();
    TouchScreenInit((uint32_t)cpuFreq.lo);
    TouchScreenCallbackSet(WidgetPointerMessage);
}

Void UiStart() {

    BIOS_getCpuFreq(&cpuFreq);
    initScreen();
    initTouch();
    System_printf("test1\n");
    System_flush();
    char tempStr[40];
    sprintf(tempStr, "\n\n\n\n START THREAD\n");
    UART_write(uart, tempStr, strlen(tempStr));

    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = 319;
    sRect.i16YMax = 239;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);

    while (1) {
        Task_sleep(1000);
        GPIO_toggle(Board_LED0);

    }
}
