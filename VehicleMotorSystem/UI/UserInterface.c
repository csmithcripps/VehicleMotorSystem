#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <xdc/std.h>
#include <ti/drivers/GPIO.h>
#include <ti/sysbios/knl/Task.h>
#include "Board.h"

Void UItest() {
    while (1) {
        Task_sleep(1000);
        GPIO_toggle(Board_LED0);
    }
}
