#include "gd32vf103.h"
#include "lcd.h"
#include "max301.h"

#include "gd32vf103_i2c.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#include "max301.h"

int main(void)
{
    max301init();
    max30102_wakeup();

    while(1)
    {
        if(data_ready)
        {
            data_ready = 0;

            uint32_t red_value;
            uint32_t ir_value;

            max30102_read_fifo(&red_value, &ir_value);

            printf("RED: %lu IR: %lu\r\n", red_value, ir_value);
        }
    }
}