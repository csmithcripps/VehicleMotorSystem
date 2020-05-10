#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xdc/std.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/sysbios/knl/Task.h>
#include "Board.h"

extern UART_Handle uart;

Void UiStart() {

    char tempStr[40];
    sprintf(tempStr, "\n\n\n\n START THREAD\n");
    UART_write(uart, tempStr, strlen(tempStr));

    while (1) {
        Task_sleep(1000);
        GPIO_toggle(Board_LED0);

    }
}
