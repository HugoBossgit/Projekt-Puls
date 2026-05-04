#include "max301.h"

#define MAX30102_ADDR (0x57 << 1)

#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C // Red LED
#define REG_LED2_PA 0x0D // IR LED
#define REG_PART_ID 0xFF
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12

#define I2C_TIMEOUT 100000

volatile uint8_t data_ready = 0;

uint32_t last_beat_time = 0;
uint16_t current_bpm = 0;
uint32_t ir_value = 0;

#define THRESHOLD 50000

uint8_t peak_detected = 0;
uint32_t last_ir_value = 0;

/* --------------------------------------------------
   Initierar I2C + interruptpinne
-------------------------------------------------- */
void max301init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C0);

    gpio_init(GPIOB,
              GPIO_MODE_AF_OD,
              GPIO_OSPEED_50MHZ,
              GPIO_PIN_6 | GPIO_PIN_7);

    i2c_clock_config(I2C0, 100000, I2C_DTCY_2);
    i2c_enable(I2C0);

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(GPIOA,
              GPIO_MODE_IPU,
              GPIO_OSPEED_50MHZ,
              GPIO_PIN_8);

    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA,
                            GPIO_PIN_SOURCE_8);

    exti_init(EXTI_8,
              EXTI_INTERRUPT,
              EXTI_TRIG_FALLING);

    eclic_irq_enable(EXTI5_9_IRQn, 1, 0);
}

/* --------------------------------------------------
   Skriver ett register
   OBS: om ni redan har denna i annan fil, ta bort denna
-------------------------------------------------- */
void max30102_write_reg(uint8_t reg, uint8_t value)
{
    int timeout;

    // START
    i2c_start_on_bus(I2C0);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_SBSEND) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return;
    }

    // ADDRESS (write)
    i2c_master_addressing(I2C0, MAX30102_ADDR, I2C_TRANSMITTER);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return;
    }
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    // SEND REGISTER
    i2c_data_transmit(I2C0, reg);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_TBE) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return;
    }

    // SEND VALUE
    i2c_data_transmit(I2C0, value);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_TBE) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return;
    }

    // STOP
    i2c_stop_on_bus(I2C0);
}

/* --------------------------------------------------
   Läser ett register
-------------------------------------------------- */
uint8_t max30102_read_reg(uint8_t reg)
{
    uint8_t data;
    int timeout;

    // START (write phase)
    i2c_start_on_bus(I2C0);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_SBSEND) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return 0;
    }

    // ADDRESS (write)
    i2c_master_addressing(I2C0, MAX30102_ADDR, I2C_TRANSMITTER);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return 0;
    }
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    // SEND REGISTER ADDRESS
    i2c_data_transmit(I2C0, reg);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_TBE) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return 0;
    }

    // RESTART (read phase)
    i2c_start_on_bus(I2C0);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_SBSEND) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return 0;
    }

    // ADDRESS (read)
    i2c_master_addressing(I2C0, MAX30102_ADDR, I2C_RECEIVER);
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return 0;
    }
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    // WAIT FOR BYTE
    timeout = I2C_TIMEOUT;
    while (!i2c_flag_get(I2C0, I2C_FLAG_RBNE) && timeout--)
        ;
    if (timeout <= 0)
    {
        i2c_stop_on_bus(I2C0);
        return 0;
    }

    // READ BYTE
    data = i2c_data_receive(I2C0);

    // STOP
    i2c_stop_on_bus(I2C0);

    return data;
}

/* --------------------------------------------------
   Väcker och konfigurerar MAX30102
-------------------------------------------------- */
void max30102_wakeup(void)
{
    // Reset
    max30102_write_reg(REG_MODE_CONFIG, 0x40);
    for (volatile int i = 0; i < 1000000; i++)
        ;

    // FIFO reset
    max30102_write_reg(REG_FIFO_WR_PTR, 0x00);
    max30102_write_reg(REG_OVF_COUNTER, 0x00);
    max30102_write_reg(REG_FIFO_RD_PTR, 0x00);

    // FIFO config
    max30102_write_reg(REG_FIFO_CONFIG, 0x00);

    // SpO2 config
    max30102_write_reg(REG_SPO2_CONFIG, 0x27);

    // LED current
    max30102_write_reg(REG_LED1_PA, 0x3F);
    max30102_write_reg(REG_LED2_PA, 0x3F);

    // Slot config
    max30102_write_reg(REG_MULTI_LED_CTRL1, 0x21); // slot1=RED, slot2=IR
    max30102_write_reg(REG_MULTI_LED_CTRL2, 0x00);

    // Start SpO2 mode
    max30102_write_reg(REG_MODE_CONFIG, 0x03);

    // Enable PPG_RDY interrupt
    max30102_write_reg(REG_INTR_ENABLE_1, 0x40);
    max30102_write_reg(REG_INTR_ENABLE_2, 0x00);

    // Clear interrupts
    max30102_read_reg(REG_INTR_STATUS_1);
    max30102_read_reg(REG_INTR_STATUS_2);
}

/* --------------------------------------------------
   Kontrollerar om sensorn är vaken
-------------------------------------------------- */
uint8_t max30102_is_awake(void)
{
    uint8_t mode = max30102_read_reg(REG_MODE_CONFIG);

    if ((mode & 0x80) == 0)
    {
        return 1; // vaken
    }
    else
    {
        return 0; // sleep/shutdown
    }
}

/* --------------------------------------------------
   Läser PART ID
   MAX30102 brukar ge 0x15
-------------------------------------------------- */
uint8_t max30102_get_part_id(void)
{
    return max30102_read_reg(REG_PART_ID);
}

/* --------------------------------------------------
   Läser 6 bytes från FIFO:
   3 bytes RED + 3 bytes IR
-------------------------------------------------- */

void max30102_read_fifo(uint32_t *red, uint32_t *ir)
{
    uint8_t data[6];

    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

    i2c_start_on_bus(I2C0);
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND));

    i2c_master_addressing(I2C0, MAX30102_ADDR, I2C_TRANSMITTER);
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    i2c_data_transmit(I2C0, REG_FIFO_DATA);
    while(!i2c_flag_get(I2C0, I2C_FLAG_TBE));

    i2c_start_on_bus(I2C0);
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND));

    i2c_master_addressing(I2C0, MAX30102_ADDR, I2C_RECEIVER);
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    for(int i = 0; i < 6; i++)
    {
        while(!i2c_flag_get(I2C0, I2C_FLAG_RBNE));

        if(i == 5)
        {
            i2c_ack_config(I2C0, I2C_ACK_DISABLE);
            i2c_stop_on_bus(I2C0);
        }

        data[i] = i2c_data_receive(I2C0);
    }

    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

    *red = ((uint32_t)data[0] << 16) |
           ((uint32_t)data[1] << 8)  |
           data[2];

    *ir  = ((uint32_t)data[3] << 16) |
           ((uint32_t)data[4] << 8)  |
           data[5];

    *red &= 0x03FFFF;
    *ir  &= 0x03FFFF;
}

/* --------------------------------------------------
   Interrupt från PA8
-------------------------------------------------- */
void EXTI5_9_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_8))
    {
        data_ready = 1;
        exti_interrupt_flag_clear(EXTI_8);
    }
}
