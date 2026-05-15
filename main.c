#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"
#include <stdio.h>
#include "drivers.h"
#include "math.h"

#include "gd32vf103_i2c.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#define false 0;
#define true 1;
long amplitude = 0;

bool checkForBeat(long sample, long now)
{
    static long prev = 0;
    static long prev2 = 0;

    static long lastBeat = 0;

    // minimum tid mellan beats
    if ((now - lastBeat) < 700)
    {
        prev2 = prev;
        prev = sample;
        return false;
    }

    // peak detection:
    // prev är peak om:
    // prev > prev2 och prev > sample
    if (prev > prev2 && prev > sample)
    {
        amplitude = prev - sample;

        // ignorera små peaks (brus)
        if (amplitude > 60)
        {
            lastBeat = now;

            prev2 = prev;
            prev = sample;

            return true;
        }
    }

    prev2 = prev;
    prev = sample;

    return false;
}

int main(void)
{
    uint32_t red_value = 0;
    uint32_t ir_value = 0;

    uint8_t counter = 0;

    Lcd_Init();
    Lcd_SetType(LCD_NORMAL);
    LCD_Clear(BLACK);

    max301init();
    eclic_global_interrupt_enable();
    max30102_wakeup();

    for (volatile int i = 0; i < 3000000; i++);

    t5omsi();

    uint32_t ms_counter = 0;

    float bpm = 0;

    long lastBeat = 0;

    // smoothing (som SparkFun)
    #define RATE_SIZE 8
    float rates[RATE_SIZE] = {0};
    uint8_t rateSpot = 0;
    static float dc = 0;
    while (1)
    {
        if (t5expq())
        {
            ms_counter++; // 1 ms tick
        }

        if (data_ready)
        {
            data_ready = 0;

            max30102_read_fifo(&red_value, &ir_value);

            // ===== BEAT DETECTION =====

            dc = dc + 0.98f * ((float)ir_value - dc);

            long ac = (long)((float)ir_value - dc);
            ac *= 4;

            if (checkForBeat(ac, ms_counter))
            {
                long delta = ms_counter - lastBeat;
                lastBeat = ms_counter;

                if (delta > 0)
                {
                    bpm = 60.0f / (delta / 1000.0f);

                    // rimlighetsfilter
                    if (bpm < 255 && bpm > 20)
                    {
                        rates[rateSpot++] = bpm;
                        rateSpot %= RATE_SIZE;

                        // medelvärde
                        float sum = 0;
                        for (int i = 0; i < RATE_SIZE; i++)
                            sum += rates[i];

                        bpm = sum / RATE_SIZE;
                    }
                }
            }

            // ===== DISPLAY =====
            counter++;
            if (counter >= 3)
            {
                LCD_ShowStr(0, 0, "BPM:", WHITE, TRANSPARENT);
                LCD_ShowNum(40, 0, (int)bpm, 3, GREEN);
                LCD_ShowNum(0, 20, (int)amplitude, 3, WHITE);
            }
        }
    }
}