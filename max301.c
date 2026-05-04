#include "max301.h"

volatile uint8_t data_ready = 0;

/* Variabler för matematik och tidsberäkning */
uint32_t last_beat_time = 0;
uint16_t current_bpm = 0;
uint32_t ir_value = 0; // Infrarött värde från sensorn

/* Enkla tröskelvärden för algoritmen (behöver kalibreras) */
#define THRESHOLD 50000 // Ett värde som betyder "ett finger ligger på sensorn"
uint8_t peak_detected = 0;
uint32_t last_ir_value = 0;

void max301init()   {
    /* enable clocks */
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C0);

    /* configure PB6 and PB7 as I2C pins */
    gpio_init(GPIOB, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);

    /* configure I2C0 */
    i2c_clock_config(I2C0, 100000, I2C_DTCY_2);
    i2c_enable(I2C0);

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);

    /* PA8 som input med pull-up */
    gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_8);

    /* konfigurera EXTI8 */
    exti_init(EXTI_8, EXTI_INTERRUPT, EXTI_TRIG_FALLING);

    /* koppla PA8 till EXTI8 */
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_8);

    /* enable interrupt */
    eclic_irq_enable(EXTI5_9_IRQn, 1, 0);
}

void max30102_wakeup() {
    // MODE_CONFIG register = 0x09
    // Bit 7 = SHDN (shutdown)
    // Skriv 0 för att väcka sensorn
    max30102_write_reg(0x09, 0x00);
    max30102_write_reg(0x09, 0x03);
    max30102_write_reg(0x0C, 0x1F);
    max30102_write_reg(0x0D, 0x1F);
    max30102_write_reg(0x0A, 0x27);
}

uint8_t max30102_read_reg(uint8_t reg)
{
    // Start + skriv adress
    i2c_start_on_bus(I2C0);
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND));

    i2c_master_addressing(I2C0, 0x57 << 1, I2C_TRANSMITTER);
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    // Skicka registeradress
    i2c_data_transmit(I2C0, reg);
    while(!i2c_flag_get(I2C0, I2C_FLAG_TBE));

    // Repeated start + läs
    i2c_start_on_bus(I2C0);
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND));

    i2c_master_addressing(I2C0, 0x57 << 1, I2C_RECEIVER);
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    while(!i2c_flag_get(I2C0, I2C_FLAG_RBNE));
    uint8_t data = i2c_data_receive(I2C0);

    i2c_stop_on_bus(I2C0);

    return data;
}


void EXTI5_9_IRQHandler(void)
{
    if(exti_interrupt_flag_get(EXTI_8)){

        exti_interrupt_flag_clear(EXTI_8);
    }
}

uint8_t i2c_read_register(uint8_t addr, uint8_t reg)
{
    i2c_start_on_bus(I2C0);
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND));

    i2c_master_addressing(I2C0, addr, I2C_TRANSMITTER);
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    i2c_data_transmit(I2C0, reg);
    while(!i2c_flag_get(I2C0, I2C_FLAG_TBE));

    i2c_start_on_bus(I2C0);
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND));

    i2c_master_addressing(I2C0, addr, I2C_RECEIVER);
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    while(!i2c_flag_get(I2C0, I2C_FLAG_RBNE));
    uint8_t data = i2c_data_receive(I2C0);

    i2c_stop_on_bus(I2C0);

    return data;
}
