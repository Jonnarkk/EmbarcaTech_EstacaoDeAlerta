/*
 *  Por: Wilton Lacerda Silva
 *  Data: 10/05/2025
 *
 *  Exemplo do uso de Filas queue no FreeRTOS com Raspberry Pi Pico
 *
 *  Descrição: Leitura do valor do joystick e exibição no display OLED SSD1306
 *  com comunicação I2C. O valor do joystick é lido a cada 100ms e enviado para a fila.
 *  A task de exibição recebe os dados da fila e atualiza o display a cada 100ms.
 *  Os leds são controlados por PWM, com brilho proporcional ao desvio do joystick.
 *  O led verde controla o eixo X e o led azul controla o eixo Y.
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/led_matriz.h"
#include "pio_matriz.pio.h"
#include "lib/font.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/bootrom.h"
#include <stdio.h>

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_JOYSTICK_X 26
#define ADC_JOYSTICK_Y 27
#define LED_RED 13
#define LED_GREEN  11
#define BUZZER 10
#define tam_quad 10
#define botaoB 6

// Variáveis globais
ssd1306_t ssd;
volatile bool modo = true;
bool cor = true;

typedef struct
{
    uint16_t x_chuva;
    uint16_t y_nivel;
} joystick_data_t;

QueueHandle_t xQueueJoystickData;

void vJoystickTask(void *params)
{
    adc_gpio_init(ADC_JOYSTICK_Y);
    adc_gpio_init(ADC_JOYSTICK_X);
    adc_init();

    joystick_data_t joydata;

    while (true)
    {
        adc_select_input(0); // GPIO 26 = ADC0
        joydata.y_nivel = adc_read();

        adc_select_input(1); // GPIO 27 = ADC1
        joydata.x_chuva = adc_read();

        xQueueSend(xQueueJoystickData, &joydata, 0); // Envia o valor do joystick para a fila
        vTaskDelay(pdMS_TO_TICKS(100));              // 10 Hz de leitura
    }
}


void vDisplayTask(void *params)
{

    joystick_data_t joydata;
    
    while (true)
    {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)
        {
            uint16_t porcX = joydata.x_chuva * 100 / 4095 ;
            uint16_t porcY = joydata.y_nivel * 100 / 4095 ;
            char str_nivel[5];
            char str_chuva[5];

            sprintf(str_chuva, "%d", porcX);
            sprintf(str_nivel, "%d", porcY);

            if(joydata.x_chuva >= 3480 || joydata.y_nivel >= 3071){
                ssd1306_fill(&ssd, !cor);            // Limpa a tela
                ssd1306_draw_string(&ssd, "ALERTA!", 30, 15);
                ssd1306_draw_string(&ssd, "NIVEIS ANORMAIS", 5, 25);
            }
            else{
                ssd1306_fill(&ssd, !cor);            // Limpa a tela
                ssd1306_draw_string(&ssd, "Niveis normais", 15, 20);
            }
            ssd1306_draw_string(&ssd, str_chuva, 30, 35);
            ssd1306_draw_string(&ssd, "%", 50, 35);
            ssd1306_send_data(&ssd);
        }
    }
}


void vLedGreenTask(void *params)
{
    joystick_data_t joydata;
    while (true)
    {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)
        {
            printf("leitura feita. Valor de X : %d\n", joydata.x_chuva);
            if(joydata.x_chuva >= 3480){
                gpio_put(LED_GREEN, true);
                modo = true;
            }
            else{
                gpio_put(LED_GREEN, false);
                modo = false;

            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Atualiza a cada 50ms
    }
}

void vLedBlueTask(void *params)
{
    joystick_data_t joydata;

    while (true)
    {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)
        {
            printf("leitura feita. Valor de Y : %d\n", joydata.y_nivel);
            if(joydata.y_nivel >= 3070){
                gpio_put(LED_RED, true);
                modo = true;
            }
            else{
                gpio_put(LED_RED, false);
                modo = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Atualiza a cada 50ms
    }
}


// Modo BOOTSEL com botão B
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

void setup(){
    // Inicializa LED's
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, false);
    
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, false);

    // Ativa BOOTSEL via botão
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicializa o I2C do display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

}

int main()
{   

    setup();
    stdio_init_all();

    // Cria a fila para compartilhamento de valor do joystick
    xQueueJoystickData = xQueueCreate(5, sizeof(joystick_data_t));

    // Criação das tasks
    xTaskCreate(vJoystickTask, "Joystick Task", 256, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);
    xTaskCreate(vLedGreenTask, "LED red Task", 256, NULL, 1, NULL);
    xTaskCreate(vLedBlueTask, "LED blue Task", 256, NULL, 1, NULL);
    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
