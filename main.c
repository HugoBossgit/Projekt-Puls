#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"
#include <stdio.h>

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

	LCD_ShowStr(0, 0, (const u8 *)"START", WHITE, TRANSPARENT);

	max301init();
	max30102_wakeup();

	for (volatile int i = 0; i < 3000000; i++)
		;

	while (1)
	{
		max30102_read_fifo(&red_value, &ir_value);

		LCD_Clear(BLACK);

		LCD_ShowStr(0, 0, (const u8 *)"RED", WHITE, TRANSPARENT);
		LCD_ShowNum(50, 0, red_value, 6, WHITE);

		LCD_ShowStr(0, 20, (const u8 *)"IR", WHITE, TRANSPARENT);
		LCD_ShowNum(50, 20, ir_value, 6, WHITE);

		for (volatile int i = 0; i < 500000; i++)
			;
		for (int i = 0; i < 32; i++)
		{
			uint32_t r, ir;
			max30102_read_fifo(&r, &ir);
		}
	}
}