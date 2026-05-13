#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"
#include <stdio.h>
#include "drivers.h"

#include "gd32vf103_i2c.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#define NO_CONTACT_LIMIT 1500

#define MIN_PEAK_DISTANCE 750

#define MIN_AMPLITUDE 350
#define MIN_PEAK_VALUE 200
#define MAX_VALLEY_VALUE -150

#define FALL_AMOUNT 80
#define RISE_FROM_VALLEY 100

#define BPM_AVG_ALPHA 0.25f

float dc = 0;
float filtered = 0;
float last_good_filtered = 0;

float valley = 0;
float peak = 0;

uint32_t peak_time = 0;
uint32_t last_peak_time = 0;
uint32_t delta_ms = 0;
uint32_t peak_count = 0;

float bpm = 0;

int state = 0;
int initialized = 0;

// state 0 = letar dal
// state 1 = letar topp efter dal

float calculate_bpm(uint32_t delta)
{
    float new_bpm;

    if (delta == 0)
    {
        return bpm;
    }

    new_bpm = 60000.0f / delta;

    if (bpm == 0)
    {
        bpm = new_bpm;
    }
    else
    {
        bpm = bpm * (1.0f - BPM_AVG_ALPHA) + new_bpm * BPM_AVG_ALPHA;
    }

    return bpm;
}

void reset_pulse_detector(void)
{
    dc = 0;
    filtered = 0;
    last_good_filtered = 0;

    valley = 0;
    peak = 0;

    peak_time = 0;
    last_peak_time = 0;
    delta_ms = 0;
    peak_count = 0;

    bpm = 0;

    state = 0;
    initialized = 0;
}

void print_bpm_lcd(uint32_t ir_value)
{
    char buffer[20];

    LCD_Clear(BLACK);

    if (ir_value < NO_CONTACT_LIMIT)
    {
        LCD_ShowStr(10, 30,
                    (const u8 *)"NO CONTACT",
                    WHITE,
                    TRANSPARENT);

        return;
    }

    LCD_ShowStr(10, 5,
                (const u8 *)"PULSE SENSOR",
                WHITE,
                TRANSPARENT);

    LCD_ShowStr(0, 30,
                (const u8 *)"BPM:",
                WHITE,
                TRANSPARENT);

    if (bpm == 0)
    {
        LCD_ShowStr(60, 30,
                    (const u8 *)"--",
                    WHITE,
                    TRANSPARENT);
    }
    else
    {
        sprintf(buffer, "%3d", (int)bpm);

        LCD_ShowStr(60, 30,
                    (const u8 *)buffer,
                    WHITE,
                    TRANSPARENT);
    }

    LCD_ShowStr(0, 55,
                (const u8 *)"Delta:",
                WHITE,
                TRANSPARENT);

    LCD_ShowNum(70, 55,
                delta_ms,
                5,
                WHITE);

    LCD_ShowStr(0, 80,
                (const u8 *)"Peaks:",
                WHITE,
                TRANSPARENT);

    LCD_ShowNum(70, 80,
                peak_count,
                5,
                WHITE);
}

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

            if (ir_value > NO_CONTACT_LIMIT)
            {
                dc = dc * 0.98f + ir_value * 0.02f;
                filtered = ir_value - dc;

                if (filtered > 2000 || filtered < -2000)
                {
                    filtered = last_good_filtered;
                }
                else
                {
                    last_good_filtered = filtered;
                }

                if (!initialized)
                {
                    valley = filtered;
                    peak = filtered;
                    initialized = 1;
                    state = 0;
                }

                if (state == 0)
                {
                    if (filtered < valley)
                    {
                        valley = filtered;
                    }

                    if (filtered > valley + RISE_FROM_VALLEY)
                    {
                        state = 1;
                        peak = filtered;
                        peak_time = ms_counter;
                    }
                }
                else if (state == 1)
                {
                    if (filtered > peak)
                    {
                        peak = filtered;
                        peak_time = ms_counter;
                    }

                    if (filtered < peak - FALL_AMOUNT)
                    {
                        float amplitude = peak - valley;
                        uint32_t diff = peak_time - last_peak_time;

                        if (amplitude > MIN_AMPLITUDE &&
                            peak > MIN_PEAK_VALUE &&
                            valley < MAX_VALLEY_VALUE)
                        {
                            if (last_peak_time == 0 || diff > MIN_PEAK_DISTANCE)
                            {
                                delta_ms = diff;
                                peak_count++;

                                if (last_peak_time != 0)
                                {
                                    bpm = calculate_bpm(delta_ms);
                                }

                                last_peak_time = peak_time;
                            }
                        }

                        state = 0;
                        valley = filtered;
                    }
                }
            }
            else
            {
                reset_pulse_detector();
            }

            if (counter >= 10)
            {
                counter = 0;
                print_bpm_lcd(ir_value);
            }
        }
    }
}