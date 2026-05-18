#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"
#include <stdio.h>
#include "drivers.h"
#include "math.h"

#include "gd32vf103_i2c.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#define false 0
#define true 1

#define RATE_SIZE 8

long amplitude = 0;

void DrawSegment(int x, int y, int w, int h, u16 color)
{
    LCD_Fill(x, y, x + w, y + h, color);
}

void DrawBigDigit(int x, int y, int digit, u16 color)
{
    int t = 4;
    int l = 20;

    int A = 0, B = 0, C = 0, D = 0, E = 0, F = 0, G = 0;

    switch (digit)
    {
    case 0: A = B = C = D = E = F = 1; break;
    case 1: B = C = 1; break;
    case 2: A = B = G = E = D = 1; break;
    case 3: A = B = G = C = D = 1; break;
    case 4: F = G = B = C = 1; break;
    case 5: A = F = G = C = D = 1; break;
    case 6: A = F = G = E = C = D = 1; break;
    case 7: A = B = C = 1; break;
    case 8: A = B = C = D = E = F = G = 1; break;
    case 9: A = B = C = D = F = G = 1; break;
    }

    if (A) DrawSegment(x + t,     y,           l, t, color);
    if (B) DrawSegment(x + l + t, y + t,       t, l, color);
    if (C) DrawSegment(x + l + t, y + l + 2*t, t, l, color);
    if (D) DrawSegment(x + t,     y + 2*l + 2*t, l, t, color);
    if (E) DrawSegment(x,         y + l + 2*t, t, l, color);
    if (F) DrawSegment(x,         y + t,       t, l, color);
    if (G) DrawSegment(x + t,     y + l + t,   l, t, color);
}

void DrawBigNumber(int x, int y, int number, u16 color)
{
    if (number < 0)
        number = 0;

    if (number > 999)
        number = 999;

    int hundreds = number / 100;
    int tens = (number / 10) % 10;
    int ones = number % 10;

    DrawBigDigit(x, y, hundreds, color);
    DrawBigDigit(x + 35, y, tens, color);
    DrawBigDigit(x + 70, y, ones, color);
}

bool checkForBeat(long sample, long now)
{
    static long prev = 0;
    static long prev2 = 0;
    static long lastBeat = 0;

    if ((now - lastBeat) < 700)
    {
        prev2 = prev;
        prev = sample;
        return false;
    }

    if (prev > prev2 && prev > sample)
    {
        amplitude = prev - sample;

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

    float rates[RATE_SIZE] = {0};
    uint8_t rateSpot = 0;

    static float dc = 0;

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

                    if (bpm < 255 && bpm > 20)
                    {
                        rates[rateSpot++] = bpm;
                        rateSpot %= RATE_SIZE;

                        float sum = 0;

                        for (int i = 0; i < RATE_SIZE; i++)
                        {
                            sum += rates[i];
                        }

                        bpm = sum / RATE_SIZE;
                    }
                }
            }

            counter++;

            if (counter >= 3)
            {
                counter = 0;

                LCD_Fill(0, 0, 160, 80, BLACK);

                LCD_ShowStr(5, 0, "BPM", WHITE, TRANSPARENT);

                DrawBigNumber(10, 18, (int)bpm, GREEN);

                LCD_ShowStr(5, 68, "A:", WHITE, TRANSPARENT);
                LCD_ShowNum(25, 68, (int)amplitude, 4, WHITE);
            }
        }
    }
}