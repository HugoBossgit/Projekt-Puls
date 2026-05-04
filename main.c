#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"
#include <stdio.h>

#include "gd32vf103_i2c.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#define IR_THRESHOLD 50000

#include "gd32vf103.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#include "gd32vf103.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#include "gd32vf103.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#define THRESHOLD 20000

int main(void)
{
    Lcd_Init();
    max301init();
    max30102_wakeup();

    Lcd_SetType(LCD_NORMAL);
    LCD_Clear(BLACK);

    uint8_t id = max30102_get_part_id();
    char buf[16];
    sprintf(buf, "ID=%02X", id);
    LCD_ShowStr(0, 16, buf, WHITE, TRANSPARENT);

    // töm FIFO en gång
    for (int i = 0; i < 32; i++) {
        uint32_t r, ir;
        max30102_read_fifo(&r, &ir);
    }

    LCD_ShowStr(0, 0, "Red: ", WHITE, TRANSPARENT);

    while(1)
    {
        //if (data_ready) {
            data_ready = 0;

            uint32_t red, ir;
            max30102_read_fifo(&red, &ir);

            LCD_ShowNum(40, 0, red, 6, WHITE);
        //}
    }
}


