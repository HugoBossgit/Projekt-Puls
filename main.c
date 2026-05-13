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
	uint8_t stat = 0;

	Lcd_Init();
	Lcd_SetType(LCD_NORMAL);
	LCD_Clear(BLACK);

	//LCD_ShowStr(0, 0, (const u8 *)"START", WHITE, TRANSPARENT);

	max301init();
	eclic_global_interrupt_enable();
	max30102_wakeup();

	for (volatile int i = 0; i < 3000000; i++);

	
	static uint8_t counter = 0;
	static uint32_t ms_counter = 0;
    t5omsi();  

    while(1)
    {
        if(t5expq())
        {
            ms_counter++;
        }
		if(data_ready)
		{
			data_ready = 0;

			max30102_read_fifo(&red_value, &ir_value);

			counter++;

			if(counter >= 5)
			{
				counter = 0;

				LCD_Clear(BLACK);

				LCD_ShowStr(0, 0, (const u8 *)"TIME:", WHITE, TRANSPARENT);
				LCD_ShowNum(0, 40, ms_counter, 6, WHITE);

				/*if(ir_value < 1500)
				{
					LCD_ShowStr(10, 20, (const u8 *)"NO MEASUREMENT", WHITE, TRANSPARENT);
				}
				else
				{
					 LCD_ShowNum(40, 40, ir_value, 6, WHITE);
				}*/
			}
		}
	}
}