#include "led_matriz.h"

Pixel desenho[NUM_PIXELS] = {0}; // Array global que representa a matriz de LEDs

// Função para limpar todos os LEDs (preto)
void limpar_todos_leds() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        desenho[i].r = 0.0;
        desenho[i].g = 0.0;
        desenho[i].b = 0.0;
    }
}

// Função para desenhar uma exclamação
void exclamacao() {
    limpar_todos_leds();
    
    // Padrão da exclamação (5x5) - vermelho
    int exclama[] = {
        0, 0, 1, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 1, 0, 0
    };

    for (int i = 0; i < NUM_PIXELS; i++) {
        if (exclama[i]) {
            set_pixel_color(i, 0.1, 0.0, 0.0); // Verde (R=0, G=0, B=0)
        }
    }
}

// Função para desenhar um V de OK
void checkmark() {
    limpar_todos_leds();
    
    // Padrão do V (5x5) - verde
    int checkmark[] = {
        0, 0, 0, 0, 0,
        1, 0, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 0, 1, 0, 1,
        0, 1, 0, 0, 0
    };

    for (int i = 0; i < NUM_PIXELS; i++) {
        if (checkmark[i]) {
            set_pixel_color(i, 0.0, 0.1, 0.0); // Verde (R=0, G=0, B=0)
        }
    }
}

// Função existente para definir a cor de um LED específico
void set_pixel_color(int led_index, double r, double g, double b) {
    if (led_index >= 0 && led_index < NUM_PIXELS) {
        desenho[led_index].r = r;
        desenho[led_index].g = g;
        desenho[led_index].b = b;
    }
}

// Função existente para converter RGB em valor de 32 bits (formato GRB)
uint32_t matrix_rgb(double b, double r, double g) {
    return ((uint32_t)(g * 255) << 24) | ((uint32_t)(r * 255) << 16) | (uint32_t)(b * 255) << 8;
}

// Função existente para enviar os dados para a matriz de LEDs
void desenho_pio(uint32_t valor_led, PIO pio, uint sm) {
    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        valor_led = matrix_rgb(
            desenho[24-i].b,  // azul
            desenho[24-i].r,  // vermelho
            desenho[24-i].g   // verde
        );
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}