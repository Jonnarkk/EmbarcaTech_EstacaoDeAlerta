#ifndef LED_MATRIZ_H
#define LED_MATRIZ_H

#include "hardware/pio.h"

#define pino_matriz 7
#define NUM_PIXELS 25

typedef struct {
    double r;
    double g;
    double b;
} Pixel;

// Protótipos das funções existentes
uint32_t matrix_rgb(double b, double r, double g);
void desenho_pio(uint32_t valor_led, PIO pio, uint sm);
void set_pixel_color(int led_index, double r, double g, double b);

// Novas funções para desenhar símbolos
void limpar_todos_leds();         // Limpa todos os LED's
void exclamacao();                 // Liga os LED's em forma de exclamação
void checkmark();                 // Liga os LED's em forma de V para dar OK

#endif // LED_MATRIZ_H