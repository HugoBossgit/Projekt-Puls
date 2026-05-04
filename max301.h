#ifndef max301_H
#define max301_H

#include "gd32vf103.h"
#include "lcd.h"
#include <stdio.h>
#include "max301.h"

void max301init();
void max30102_wakeup();
void calculate_BPM(uint32_t);
uint8_t i2c_read_register(uint8_t, uint8_t);
uint8_t max30102_read_reg(uint8_t);

#endif