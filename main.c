#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"
#include <stdio.h>
#include "drivers.h"
#include "math.h"

#include "gd32vf103_i2c.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"


int main(void)
{
    uint32_t red_value = 0;
    uint32_t ir_value = 0;

    float bpm = 0;
    static float ir_avg = 0;
    static int last_ir = 0;
    static int peak_detected = 0;
    static uint32_t last_peak_time = 0;
    static float dynamic_threshold = 0;

    uint8_t counter = 0;
    static uint32_t ms_counter = 0;

    Lcd_Init();
    Lcd_SetType(LCD_NORMAL);
    LCD_Clear(BLACK);

    max301init();
    eclic_global_interrupt_enable();
    max30102_wakeup();

    for (volatile int i = 0; i < 3000000; i++);

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

            ir_avg = ir_avg * 0.95 + ir_value * 0.05;   // lågpass
            float ir_ac = ir_value - ir_avg;            // högpass

            dynamic_threshold = 0.9f * dynamic_threshold + 0.1f * fabs(ir_ac);

            if (ir_ac > threshold && last_ir <= threshold) {
                // Ny topp
                uint32_t now = ms_counter;
                uint32_t dt = now - last_peak_time;

                if (dt > 300) { // ignorera falska toppar (<200 BPM)
                    bpm = 60000.0f / dt;
                    last_peak_time = now;
                }
            }

            last_ir = ir_ac;

            if (counter >= 10)
            {
                counter = 0;

                LCD_ShowNum(20, 0, ir_value, 5, WHITE);
                LCD_ShowNum(20, 20, (int)bpm, 3, GREEN);
            }
        }
    }
}