#include <stdint.h>
#include <stm32f10x.h>

typedef struct {
    uint32_t delay;
    int index;
} frequency_t;

const frequency_t frequencies[] = {
    {64000000, -6},
    {32000000, -5},
    {16000000, -4},
    {8000000,  -3},
    {4000000,  -2},
    {2000000,  -1},
    {1000000,   0},
    {500000,    1},
    {250000,    2},
    {125000,    3},
    {62500,     4},
    {31250,     5},
    {15625,     6}
};

const int NUM_FREQUENCIES = 13;
int current_freq_index = 6;
int frequency_changed = 0;

void delay(uint32_t ticks) {
    for (volatile uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

void init_led(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_0;
}

void init_buttons(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_CNF1);
    GPIOB->CRL |= (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1);
    GPIOB->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BS1;
}

void handle_buttons(void) {

    if (!(GPIOB->IDR & GPIO_IDR_IDR0)) {
        if (current_freq_index < NUM_FREQUENCIES - 1) {
            current_freq_index++;
            frequency_changed = 1;
        }
        while (!(GPIOB->IDR & GPIO_IDR_IDR0)) {
            delay(10000);
        }
        return;
    }
    

    if (!(GPIOB->IDR & GPIO_IDR_IDR1)) {
        if (current_freq_index > 0) {
            current_freq_index--;
            frequency_changed = 1;
        }
        while (!(GPIOB->IDR & GPIO_IDR_IDR1)) {
            delay(10000);
        }
        return;
    }
}

int main(void) {
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    init_led();
    init_buttons();
    
    while (1) {
        handle_buttons();
        if (frequency_changed) {
            for (int i = 0; i < 3; i++) {
                GPIOC->ODR |= GPIO_ODR_ODR13;
                delay(100000);
                GPIOC->ODR &= ~GPIO_ODR_ODR13;
                delay(100000);
            }
            frequency_changed = 0;
        }
        
        GPIOC->ODR |= GPIO_ODR_ODR13;
        delay(frequencies[current_freq_index].delay / 2);
        
        GPIOC->ODR &= ~GPIO_ODR_ODR13;
        delay(frequencies[current_freq_index].delay / 2);
    }
}
