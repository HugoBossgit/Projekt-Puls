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

int main(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);

    while(1)
    {
        gpio_bit_reset(GPIOC, GPIO_PIN_13); // låg
        for(volatile int i = 0; i < 1000000; i++);

        gpio_bit_set(GPIOC, GPIO_PIN_13);   // hög
        for(volatile int i = 0; i < 1000000; i++);
    }
}