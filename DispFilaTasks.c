#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/buzzer.h"
#include "lib/led_matriz.h"
#include "pio_matriz.pio.h"
#include "lib/font.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/bootrom.h"
#include <stdio.h>

// Defines dos periféricos
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_JOYSTICK_X 26
#define ADC_JOYSTICK_Y 27
#define LED_RED 13
#define LED_GREEN  11
#define BUZZER 10
#define botaoB 6

// Variáveis globais
ssd1306_t ssd;                  // Variável referente ao display
bool cor = true;                // Variável booleana para habilitar a impressão no display

typedef struct // Declaração de tipo para coleta dos dados do joystick
{
    uint16_t x_chuva;
    uint16_t y_nivel;
} joystick_data_t;

QueueHandle_t xQueueJoystickData;   // Criação da fila do FreeRTOS

// Função da tarefa para leitura do joystick
void vJoystickTask(void *params)
{
    adc_gpio_init(ADC_JOYSTICK_Y);
    adc_gpio_init(ADC_JOYSTICK_X);
    adc_init();

    joystick_data_t joydata;  

    while (true) // Loop para leitura dos valores do ADC
    {
        adc_select_input(0); // GPIO 26 = ADC0
        joydata.y_nivel = adc_read();

        adc_select_input(1); // GPIO 27 = ADC1
        joydata.x_chuva = adc_read();

        xQueueSend(xQueueJoystickData, &joydata, 0); // Envia o valor do joystick para a fila
        vTaskDelay(pdMS_TO_TICKS(100));              // 10 Hz de leitura
    }
}

// Função da tarefa do display - Funções da matriz estão no arquivo ssd1306.c
void vDisplayTask(void *params)
{

    joystick_data_t joydata;
    
    while (true)
    {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE) // Verificação de presença de dados na fila
        {
            uint16_t porcX = joydata.x_chuva * 100 / 4095 ;     // Variável para impressão da porcentagem do volume de chuva no display
            uint16_t porcY = joydata.y_nivel * 100 / 4095 ;     // Variável para impressão da porcentagem do nível de água
            char str_nivel[5];
            char str_chuva[5];
            
            // Transformação das porcentagens em string
            sprintf(str_chuva, "%d", porcX);
            sprintf(str_nivel, "%d", porcY);
            
            if(joydata.x_chuva >= 3480 || joydata.y_nivel >= 3071){             // Verificação do limiar estipulado para níveis críticos
                ssd1306_fill(&ssd, !cor);                                       // Limpa a tela
                ssd1306_draw_string(&ssd, "ALERTA!", centralizar_texto("ALERTA!"), 5);                       // Mostra texto no display                          
                ssd1306_draw_string(&ssd, "NIVEIS ANORMAIS", centralizar_texto("NIVEIS ANORMAIS"), 15);      // Mostra texto no display  
            }
            else{
                ssd1306_fill(&ssd, !cor);                   // Limpa a tela
                ssd1306_draw_string(&ssd, "Niveis normais", centralizar_texto("Niveis Normais"), 15); // Mostra texto no display
            }
            ssd1306_draw_string(&ssd, "V. chuva:", 10, 35);         // Mostra texto Volume de chuva no display
            ssd1306_draw_string(&ssd, str_chuva, 90, 35);           // Mostra porcentagem númerica 
            ssd1306_draw_string(&ssd, "%", 110, 35);                // Mostra símbolo de porcentagem
            ssd1306_draw_string(&ssd, "N. agua:", 10, 45);          // Mostra texto Nível de água no display
            ssd1306_draw_string(&ssd, str_nivel, 90, 45);           // Mostra porcentagem númerica 
            ssd1306_draw_string(&ssd, "%", 110, 45);                // Mostra símbolo de porcentagem
            ssd1306_send_data(&ssd);                                // Envia dados para o display
        }
    }
}

// Função da tarefa do LED RGB
void vLedTask(void *params)
{
    joystick_data_t joydata;
    while (true)
    {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)   // Verificação de presença de dados na fila
        {
            printf("leitura feita. Valor de X : %d\n", joydata.x_chuva);            // Imprime mensagem na comunicação serial para debug
            printf("leitura feita. Valor de Y : %d\n", joydata.y_nivel);            // Imprime mensagem na comunicação serial para debug
            if(joydata.x_chuva >= 3480 || joydata.y_nivel >= 3070){                 // Verificação do limiar estipulado para níveis críticos
                gpio_put(LED_GREEN, false);
                gpio_put(LED_RED, true);
            }
            else{
                gpio_put(LED_GREEN, true);
                gpio_put(LED_RED, false);

            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Atualiza a cada 50ms
    }
}

// Função da tarefa da matriz de LED's - Funções estão no arquivo led_matriz.c
void vMatrizTask(void *params){
    // Declaração de variáveis utilizadas na matriz de LED's
    joystick_data_t joydata;
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &pio_matriz_program);
    pio_matriz_program_init(pio, sm, offset, pino_matriz);

    while(true){
        if(xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE){   // Verificação de presença de dados na fila
            if(joydata.x_chuva >= 3480 || joydata.y_nivel >= 3071){                 // Verificação do limiar estipulado para níveis críticos
                exclamacao();                   // Desenha exclamação na matriz de LED's
                desenho_pio(0, pio, sm);        // Envia os dados para a matriz
            }
            else{
                checkmark();                    // Desenha um checkmark na matriz de LED's
                desenho_pio(0, pio, sm);        // Envia os dados para a matriz
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));          // Atualiza a cada 50ms
    }
}

// Função da tarefa do buzzer
void vBuzzerTask(void *params){
    joystick_data_t joydata;

    while (true)
    {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE){  // Verificação de presença de dados na fila
            if(joydata.x_chuva >= 3480 || joydata.y_nivel >= 3070){                 // Verificação do limiar estipulado para níveis críticos
                buzz(BUZZER, 600, 500);                                             // Função para acionar o buzzer - chama função no arquivo buzzer.c
                    for(int i = 0; i < 10; i++)                                     // For loop para delay de 100 ms quebrado em pequenas intervalos
                        vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));          // Atualiza a cada 50ms
    }
}

// Modo BOOTSEL com botão B - Limpa Display & Matriz
void gpio_irq_handler(uint gpio, uint32_t events)
{   
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &pio_matriz_program);
    pio_matriz_program_init(pio, sm, offset, pino_matriz);

    // Limpa Display
    ssd1306_fill(&ssd, !cor);
    ssd1306_send_data(&ssd);

    // Limpa matriz de LED's
    limpar_todos_leds();
    desenho_pio(0, pio, sm);

    // Põe em modo bootsel
    reset_usb_boot(0, 0);
}

// Função para inicialização dos periféricos
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

    // Inicializa o buzzer
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER,GPIO_OUT);
    

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

    setup();            // Chama função para setup inicial dos periféricos
    stdio_init_all();

    // Cria a fila para compartilhamento de valor do joystick
    xQueueJoystickData = xQueueCreate(5, sizeof(joystick_data_t));

    // Criação das tasks
    xTaskCreate(vJoystickTask, "Joystick Task", 256, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display Task", 512, NULL, 1, NULL);
    xTaskCreate(vLedTask, "LED red Task", 256, NULL, 1, NULL);
    xTaskCreate(vMatrizTask, "Matriz Task", 256, NULL, 1, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", 256, NULL, 1, NULL);
    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}
