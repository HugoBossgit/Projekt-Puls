#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"
#include <stdio.h>
#include "drivers.h"

#include "gd32vf103_i2c.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"


int main(void)
{
    uint32_t red_value = 0;
    uint32_t ir_value = 0;

    uint8_t counter = 0;
    static uint32_t ms_counter = 0;

    Lcd_Init();
    Lcd_SetType(LCD_NORMAL);
    LCD_Clear(BLACK);

    max301init();
    eclic_global_interrupt_enable();
    max30102_wakeup();

    for (volatile int i = 0; i < 3000000; i++)
        ;

    t5omsi();

    while (1)
    {
        if (t5expq())
        {
            ms_counter++;
        }

        if (data_ready)
        {
            data_ready = 0;

            max30102_read_fifo(&red_value, &ir_value);

            counter++;

            if (counter >= 10)
            {
                counter = 0;
                LCD_ShowNum(70, 55, ir_value, 5,WHITE);
            }
        }
    }
}