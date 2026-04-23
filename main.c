//GPIO PIN 13 = RED ---- GPIO PIN 1 = GREEN ---- GPIO PIN 2 = BLUE

#include "gd32vf103.h"

int main(){
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOC);
	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_2);
	gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);
	while(1){
		gpio_bit_reset(GPIOC, GPIO_PIN_13);
		gpio_bit_set(GPIOA, GPIO_PIN_1 | GPIO_PIN_2);
		for(volatile int i = 0; i < 5000000; i++);
		gpio_bit_reset(GPIOA, GPIO_PIN_1);
		gpio_bit_set(GPIOA, GPIO_PIN_2);
		gpio_bit_set(GPIOC, GPIO_PIN_13);
		for(volatile int i = 0; i < 5000000; i++);
		gpio_bit_reset(GPIOA, GPIO_PIN_2);
		gpio_bit_set(GPIOA, GPIO_PIN_1);
		gpio_bit_set(GPIOC, GPIO_PIN_13);
		for(volatile int i = 0; i < 5000000; i++);
	}

}
