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
    GPIOC->CRH |= GPIO_CRH_MODE13_0;  // PC13 as output
}

void init_buttons(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_CNF1);
    GPIOB->CRL |= (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1); // input pull-up

    GPIOB->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BS1;  // enable pull-ups
}

void handle_buttons(void) {

    if (!(GPIOB->IDR & GPIO_IDR_IDR0)) {
        if (current_freq_index < NUM_FREQUENCIES - 1) {
            current_freq_index++;
            frequency_changed = 1;
        }
        while (!(GPIOB->IDR & GPIO_IDR_IDR0)) { delay(10000); }
        return;
    }

    if (!(GPIOB->IDR & GPIO_IDR_IDR1)) {
        if (current_freq_index > 0) {
            current_freq_index--;
            frequency_changed = 1;
        }
        while (!(GPIOB->IDR & GPIO_IDR_IDR1)) { delay(10000); }
        return;
    }
}



//  TIMER CONFIG

void tim2_init(uint32_t half_period_ticks)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1 = 0;
    TIM2->PSC = 64 - 1;              // 1 MHz tick (при 64 MHz системных)
    TIM2->ARR = half_period_ticks;   // половина периода
    TIM2->EGR |= TIM_EGR_UG;

    TIM2->DIER |= TIM_DIER_UIE;      // interrupt on update

    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1 |= TIM_CR1_CEN;
}

void tim2_update_period(uint32_t half_period_ticks)
{
    TIM2->ARR = half_period_ticks;
    TIM2->EGR |= TIM_EGR_UG;
}



//  TIMER IRQ HANDLER

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;

        GPIOC->ODR ^= GPIO_ODR_ODR13;   // toggle PC13
    }
}



//        MAIN

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    init_led();
    init_buttons();

    uint32_t start_delay = frequencies[current_freq_index].delay / 2;
    tim2_init(start_delay);

    while (1) {

        handle_buttons();

        if (frequency_changed) {
            for (int i = 0; i < 3; i++) {
                GPIOC->ODR |= GPIO_ODR_ODR13;
                delay(100000);
                GPIOC->ODR &= ~GPIO_ODR_ODR13;
                delay(100000);
            }

            tim2_update_period(frequencies[current_freq_index].delay / 2);
            frequency_changed = 0;
        }

        __WFI(); // ждём прерываний
    }
}
