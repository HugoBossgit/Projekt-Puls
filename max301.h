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

#ifndef MAX301_H
#define MAX301_H

#include <stdint.h>

extern volatile uint8_t data_ready;

void max301init(void);
void max30102_wakeup(void);
uint8_t max30102_is_awake(void);
uint8_t max30102_get_part_id(void);
uint8_t max30102_read_reg(uint8_t reg);
void max30102_write_reg(uint8_t reg, uint8_t value);
void max30102_read_fifo(uint32_t *red, uint32_t *ir);

#endif
#endif