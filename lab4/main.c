#include <stdint.h>
#include <stm32f10x.h>


#define OLED_SCK_PIN   5   // PA5
#define OLED_MOSI_PIN  7   // PA7
#define OLED_CS_PIN    4   // PA4
#define OLED_DC_PIN    0   // PB0
#define OLED_RST_PIN   1   // PB1

static void gpio_init(void) {

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;


    GPIOA->CRL &= ~((0xFU << (OLED_SCK_PIN  * 4)) |
                    (0xFU << (OLED_MOSI_PIN * 4)) |
                    (0xFU << (OLED_CS_PIN   * 4)));

    GPIOA->CRL |=  ((0x1U << (OLED_SCK_PIN  * 4)) |
                    (0x1U << (OLED_MOSI_PIN * 4)) |
                    (0x1U << (OLED_CS_PIN   * 4)));


    GPIOB->CRL &= ~((0xFU << (OLED_DC_PIN  * 4)) |
                    (0xFU << (OLED_RST_PIN * 4)));

    GPIOB->CRL |=  ((0x1U << (OLED_DC_PIN  * 4)) |
                    (0x1U << (OLED_RST_PIN * 4)));

    // Устанавливаем дефолтные уровни
    GPIOA->BSRR = (1U << (OLED_CS_PIN + 16)); // CS = 1 (не выбран)
    GPIOB->BSRR = (1U << OLED_RST_PIN);       // RST = 1
    GPIOB->BSRR = (1U << OLED_DC_PIN);        // DC = 1
}

static void spi1_init(void) {

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // Настройка SPI1:

    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_SSM  | SPI_CR1_SSI |
                SPI_CR1_BR_1 | SPI_CR1_BR_0;

    SPI1->CR1 |= SPI_CR1_SPE; // Включаем SPI
}

static void spi_write(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_TXE));
    while (SPI1->SR & SPI_SR_BSY);
}

static void oled_reset(void) {
    // RST = 0 → задержка → 1
    GPIOB->BSRR = (1U << (OLED_RST_PIN + 16));
    for (volatile int i = 0; i < 80000; i++);
    GPIOB->BSRR = (1U << OLED_RST_PIN);
    for (volatile int i = 0; i < 80000; i++);
}

static void oled_cmd(uint8_t cmd) {
    GPIOB->BSRR = (1U << (OLED_DC_PIN + 16)); // DC = 0 (команда)
    GPIOA->BSRR = (1U << (OLED_CS_PIN + 16)); // CS = 0
    spi_write(cmd);
    GPIOA->BSRR = (1U << OLED_CS_PIN);        // CS = 1
}

static void oled_data(uint8_t data) {
    GPIOB->BSRR = (1U << OLED_DC_PIN);        // DC = 1 (данные)
    GPIOA->BSRR = (1U << (OLED_CS_PIN + 16)); // CS = 0
    spi_write(data);
    GPIOA->BSRR = (1U << OLED_CS_PIN);        // CS = 1
}

static void oled_init(void) {
    oled_reset();
    oled_cmd(0xAE);
    oled_cmd(0x20); oled_cmd(0x00); 
    oled_cmd(0x40);
    oled_cmd(0x81); oled_cmd(0x7F);
    oled_cmd(0xA1);
    oled_cmd(0xA6);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14);
    oled_cmd(0xAF);
}

static void oled_set_cursor(uint8_t page, uint8_t col) {
    oled_cmd(0xB0 + page);                       // page start
    oled_cmd(0x00 + (col & 0x0F));               // lower nibble
    oled_cmd(0x10 + ((col >> 4) & 0x0F));        // upper nibble
}


static const uint8_t test_image[8][128] = {
    // 8 страниц * 128 байт (1 страница = 8 px по высоте)
    // Генерируем шахматку: 0xAA / 0x55 чередуем по блокам 8 px
    {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
};

static void fill_test_image(void) {
    for (int page = 0; page < 8; page++) {
        for (int col = 0; col < 128; col++) {
            int block = (col / 8) % 2;
            int rowblock = (page % 2);
            if (block ^ rowblock)
                ((uint8_t*)test_image)[page][col] = 0xAA;
            else
                ((uint8_t*)test_image)[page][col] = 0x55;
        }
    }
}

static void oled_draw_test(void) {
    fill_test_image();
    for (int page = 0; page < 8; page++) {
        oled_set_cursor(page, 0);
        for (int col = 0; col < 128; col++) {
            oled_data(test_image[page][col]);
        }
    }
}

int main(void) {
    gpio_init();
    spi1_init();
    oled_init();

    oled_draw_test();

    while (1) {
        for (volatile int i = 0; i < 1600000; i++);
    }
}
