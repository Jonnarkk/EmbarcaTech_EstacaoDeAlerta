#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/led_matriz.h"
#include "lib/sirene.h"
#include "hardware/pio.h"
#include "pio_matriz.pio.h"
#include "hardware/clocks.h"
#include <stdio.h>

#define led_pin_green 11
#define led_pin_blue 12
#define led_pin_red 13
#define botao_A 5
#define botaoB 6
#define BUZZER 10
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Variáveis globais
volatile bool modo = true;                                                          // Variável que controla o modo do semáforo
volatile bool cor = true;                                                           // Variável para utilizar funções no display
ssd1306_t ssd;                                                                      // Inicializa a estrutura do display
volatile bool verde = false, vermelho = false, amarelo = false, noturno = false;    // Variáveis para controle do Buzzer
volatile bool botao_pressionado = false;                                            // Variável para interrupção do botão A


// Handles das tasks
TaskHandle_t xTaskBlinkNormal = NULL;       // Handle para tarefa do blink do modo normal
TaskHandle_t xTaskBlinkNoturna = NULL;      // Handle para tarefa do blink do modo noturno
TaskHandle_t xTaskMatrizNormal = NULL;      // Handle para tarefa da matriz do modo normal
TaskHandle_t xTaskMatrizNoturna = NULL;     // Handle para tarefa da matriz do modo noturno
TaskHandle_t xTaskDisplayNormal = NULL;     // Handle para tarefa do display do modo normal
TaskHandle_t xTaskDisplayNoturna = NULL;    // Handle para tarefa do display do modo noturno

void vTaskControle(){
    const TickType_t xDelay = pdMS_TO_TICKS(20); // Polling a cada 20ms
    bool debounce = false;
    absolute_time_t last_press = 0;

    while(true) {
        if(botao_pressionado) {
            absolute_time_t now = get_absolute_time();
            
            // Debounce - verifica se já passou tempo suficiente
            if(absolute_time_diff_us(last_press, now) > 200000) { // 200ms
                last_press = now;
                modo = !modo; 
                
                // Gerencia as tasks conforme o estado da variável modo
                if(modo) {
                    // Modo normal
                     vTaskResume(xTaskBlinkNormal);
                    vTaskSuspend(xTaskBlinkNoturna);

                    vTaskResume(xTaskMatrizNormal);
                    vTaskSuspend(xTaskMatrizNoturna);

                    vTaskResume(xTaskDisplayNormal);
                    vTaskSuspend(xTaskDisplayNoturna);

                noturno = false;
                } else {
                    // Modo noturno
                    vTaskResume(xTaskBlinkNoturna);
                    vTaskSuspend(xTaskBlinkNormal);

                    vTaskResume(xTaskMatrizNoturna);
                    vTaskSuspend(xTaskMatrizNormal);

                    vTaskResume(xTaskDisplayNoturna);
                    vTaskSuspend(xTaskDisplayNormal);
                    noturno = true;
                    verde = amarelo = vermelho = false;
                }
            }
            
            botao_pressionado = false; // Reseta o flag
        }
        
        vTaskDelay(xDelay); // Espera para próxima verificação
    }
}

// Função do pisca para modo normal
void vBlinkTask_Normal(){

    while (true){
        // Led na cor vermelha
        vermelho = false;
        verde = true;
        gpio_put(led_pin_green, true);
        vTaskDelay(pdMS_TO_TICKS(4000));
        
        // Led na cor amarela
        verde = false;
        amarelo = true;
        gpio_put(led_pin_red, true);
        vTaskDelay(pdMS_TO_TICKS(4000));
        
        // led na cor vermelha
        amarelo = false;
        vermelho = true;
        gpio_put(led_pin_blue, false);
        gpio_put(led_pin_green, false);
        gpio_put(led_pin_red, true);
        vTaskDelay(pdMS_TO_TICKS(4000));
        gpio_put(led_pin_red, false);
        printf("Blink\n");
    }
}

// Função do pisca para modo noturno
void vBlinkTask_Noturno(){

    while (true){
        // Led na cor amarela
        gpio_put(led_pin_green, true);
        gpio_put(led_pin_red, true);
        vTaskDelay(pdMS_TO_TICKS(3000));
        gpio_put(led_pin_green, false);
        gpio_put(led_pin_red, false);
        vTaskDelay(pdMS_TO_TICKS(3000));
        printf("Blink Noturno\n");
    }
}

// Função para desenho na matriz de LED's no modo normal - Chama funções do arquivo led_matriz.c
void vMatrizTask_Normal(){
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &pio_matriz_program);
    pio_matriz_program_init(pio, sm, offset, pino_matriz);
    
    while(true){
            // Imprime desenho de seta na matriz
            desenhar_seta_direita();
            desenho_pio(0, pio, sm); // Atualiza a matriz de LEDs
            vTaskDelay(pdMS_TO_TICKS(3000));
            limpar_todos_leds();
            desenho_pio(0, pio, sm);
            vTaskDelay(pdMS_TO_TICKS(1000));

            // Imprime desenho de exclamação na matriz
            desenhar_exclamacao();
            desenho_pio(0, pio, sm); // Atualiza a matriz de LEDs
            vTaskDelay(pdMS_TO_TICKS(3000));
            limpar_todos_leds();
            desenho_pio(0, pio, sm);
            vTaskDelay(pdMS_TO_TICKS(1000));

            // Imprime desenho de proibido na matriz
            desenhar_proibido();
            desenho_pio(0, pio, sm); // Atualiza a matriz de LEDs
            vTaskDelay(pdMS_TO_TICKS(3000));
            limpar_todos_leds();
            desenho_pio(0, pio, sm);
            vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task para desenho na matriz de LED's no modo noturno - Chama funções do arquivo led_matriz.c
void vMatrizTask_Noturna(){
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &pio_matriz_program);
    pio_matriz_program_init(pio, sm, offset, pino_matriz);
    
    while(true){
            // Imprime desenho de exclamação
            desenhar_exclamacao();
            desenho_pio(0, pio, sm); // Atualiza a matriz de LEDs
            vTaskDelay(pdMS_TO_TICKS(200));
            limpar_todos_leds();
            desenho_pio(0, pio, sm);
            vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Task para desenho no display no modo normal - Chama funções do arquivo ssd1306.c
void vDisplayTask_Normal(){

    while(true){
        // Liga sinaleira na cor verde
        ssd1306_fill(&ssd, !cor);                                   // Limpa o display
        ssd1306_draw_string(&ssd, "VERDE", 10, 10);
        ssd1306_draw_string(&ssd, "AMARELO", 10, 30);
        ssd1306_draw_string(&ssd, "VERMELHO", 10, 50);
        ssd1306_rect(&ssd, 3, 90, 21, 18, cor, cor);                // Retângulo cheio
        ssd1306_rect(&ssd, 22, 90, 21, 18, cor, !cor);              // Retângulo vazio
        ssd1306_rect(&ssd, 41, 90, 21, 18, cor, !cor);              // Retângulo vazio
        ssd1306_rect(&ssd, 3, 90, 3, 56, cor, cor);                // Retângulo cheio esquerdo
        ssd1306_rect(&ssd, 3, 108, 3, 56, cor, cor);                // Retângulo cheio direito
        ssd1306_send_data(&ssd);                                    // Atualiza o display
        vTaskDelay(pdMS_TO_TICKS(3980));
        
        // Liga sinaleira na cor amarela
        ssd1306_fill(&ssd, !cor);                                   // Limpa o display
        ssd1306_draw_string(&ssd, "VERDE", 10, 10);
        ssd1306_draw_string(&ssd, "AMARELO", 10, 30);
        ssd1306_draw_string(&ssd, "VERMELHO", 10, 50);
        ssd1306_rect(&ssd, 3, 90, 21, 18, cor, !cor);                // Retângulo vazio
        ssd1306_rect(&ssd, 22, 90, 21, 18, cor, cor);              // Retângulo cheio
        ssd1306_rect(&ssd, 41, 90, 21, 18, cor, !cor);              // Retângulo vazio
        ssd1306_rect(&ssd, 3, 90, 3, 56, cor, cor);                // Retângulo cheio esquerdo
        ssd1306_rect(&ssd, 3, 108, 3, 56, cor, cor);                // Retângulo cheio direito
        ssd1306_send_data(&ssd);                                    // Atualiza o display
        vTaskDelay(pdMS_TO_TICKS(3980));

        // Liga sinaleira na cor vermelha
        ssd1306_fill(&ssd, !cor);                                   // Limpa o display
        ssd1306_draw_string(&ssd, "VERDE", 10, 10);
        ssd1306_draw_string(&ssd, "AMARELO", 10, 30);
        ssd1306_draw_string(&ssd, "VERMELHO", 10, 50);
        ssd1306_rect(&ssd, 3, 90, 21, 18, cor, !cor);                // Retângulo vazio
        ssd1306_rect(&ssd, 22, 90, 21, 18, cor, !cor);              // Retângulo vazio
        ssd1306_rect(&ssd, 41, 90, 21, 18, cor, cor);              // Retângulo cheio
        ssd1306_rect(&ssd, 3, 90, 3, 56, cor, cor);                // Retângulo cheio esquerdo
        ssd1306_rect(&ssd, 3, 108, 3, 56, cor, cor);                // Retângulo cheio direito
        ssd1306_send_data(&ssd);                                    // Atualiza o display
        vTaskDelay(pdMS_TO_TICKS(3980));
    }
}

// Task para desenho no display do modo noturno - Chama funções do arquivo ssd1306.c
void vDisplayTask_Noturno(){

    while(true){
        // Liga sinaleira na cor amarela
        ssd1306_fill(&ssd, !cor);                                   // Limpa o display
        ssd1306_draw_string(&ssd, "VERDE", 10, 10);
        ssd1306_draw_string(&ssd, "AMARELO", 10, 30);
        ssd1306_draw_string(&ssd, "VERMELHO", 10, 50);
        ssd1306_rect(&ssd, 3, 90, 21, 18, cor, !cor);                // Retângulo vazio
        ssd1306_rect(&ssd, 22, 90, 21, 18, cor, cor);              // Retângulo cheio
        ssd1306_rect(&ssd, 41, 90, 21, 18, cor, !cor);              // Retângulo vazio
        ssd1306_rect(&ssd, 3, 90, 3, 56, cor, cor);                // Retângulo cheio esquerdo
        ssd1306_rect(&ssd, 3, 108, 3, 56, cor, cor);                // Retângulo cheio direito
        ssd1306_send_data(&ssd);                                    // Atualiza o display
        vTaskDelay(pdMS_TO_TICKS(2980));

        // Desliga sinaleira
        ssd1306_fill(&ssd, !cor);                                   // Limpa o display
        ssd1306_draw_string(&ssd, "VERDE", 10, 10);
        ssd1306_draw_string(&ssd, "AMARELO", 10, 30);
        ssd1306_draw_string(&ssd, "VERMELHO", 10, 50);
        ssd1306_rect(&ssd, 3, 90, 21, 18, cor, !cor);                // Retângulo vazio
        ssd1306_rect(&ssd, 22, 90, 21, 18, cor, !cor);              // Retângulo vazio
        ssd1306_rect(&ssd, 41, 90, 21, 18, cor, !cor);              // Retângulo vazio
        ssd1306_rect(&ssd, 3, 90, 3, 56, cor, cor);                // Retângulo cheio esquerdo
        ssd1306_rect(&ssd, 3, 108, 3, 56, cor, cor);                // Retângulo cheio direito
        ssd1306_send_data(&ssd);                                    // Atualiza o display
        vTaskDelay(pdMS_TO_TICKS(2980));
    }
}


// Task do Buzzer - Chama função no arquivo sirene.c
void vBuzzerTask(){
    while(true){
        if(verde){
            sirene(150, 880, 800);
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }
        else if(amarelo){
            for(int i = 0; i < 5; i++){
                sirene(500, 400, 100);
                vTaskDelay(pdMS_TO_TICKS(10)); 
            }
        }
        else if(vermelho){
            sirene(500, 700, 500);
            vTaskDelay(pdMS_TO_TICKS(1500)); 
        }
        else if(noturno){
            sirene(300, 250, 1000);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}


// Função Handler das interrupções
void gpio_irq_handler(uint gpio, uint32_t events){
    if(gpio == botaoB){ // Coloca placa em modo bootsel & limpa matriz
        PIO pio = pio0;
        uint sm = 0;
        uint offset = pio_add_program(pio, &pio_matriz_program);
        pio_matriz_program_init(pio, sm, offset, pino_matriz);

        limpar_todos_leds();
        desenho_pio(0, pio, sm);

        ssd1306_fill(&ssd, !cor);                                   // Limpa o display
        ssd1306_send_data(&ssd);                                    // Atualiza o display
        
        reset_usb_boot(0, 0);
    }
    
    if(gpio == botao_A){
        botao_pressionado = true;
    }
}



void setup(){
    // Inicialização do LED RGB
    gpio_init(led_pin_red);
    gpio_set_dir(led_pin_red, GPIO_OUT);

    gpio_init(led_pin_blue);
    gpio_set_dir(led_pin_blue, GPIO_OUT);

    gpio_init(led_pin_green);
    gpio_set_dir(led_pin_green, GPIO_OUT);

    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL,
                                         true, &gpio_irq_handler);
    // Fim do trecho para modo BOOTSEL com botão B

    // Inicialização do botão A
    gpio_init(botao_A);
    gpio_set_dir(botao_A, GPIO_IN);
    gpio_pull_up(botao_A);
    gpio_set_irq_enabled_with_callback(botao_A, GPIO_IRQ_EDGE_FALL,
        true, &gpio_irq_handler);

     // Inicializa o PWM do buzzer
     gpio_init(BUZZER);
     gpio_set_function(BUZZER, GPIO_FUNC_PWM);

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
 
}

int main(){
   
    setup();
    stdio_init_all();

    // Tasks do Blink
    xTaskCreate(vBlinkTask_Normal, "Blink Task Normal", 
    configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskBlinkNormal);
    xTaskCreate(vBlinkTask_Noturno, "Blink Task Noturna",
    configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskBlinkNoturna); 

    // Tasks da Matriz
    xTaskCreate(vMatrizTask_Normal, "Matriz Task Normal",
    configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskMatrizNormal); 
    xTaskCreate(vMatrizTask_Noturna, "Matriz Task Noturna",
    configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskMatrizNoturna); 

    // Tasks do Display
    xTaskCreate(vDisplayTask_Normal, "Display Task Normal",
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskDisplayNormal); 
    xTaskCreate(vDisplayTask_Noturno, "Display Task Noturna",
        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskDisplayNoturna); 
    
    // Task do Buzzer
    xTaskCreate(vBuzzerTask, "Buzzer Task",
         configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL); 

    // Task de controle com alta prioridade
    xTaskCreate(vTaskControle, "Controle Task", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    // Inicialmente, suspende as tarefas do modo noturno
        vTaskSuspend(xTaskBlinkNoturna);
        vTaskSuspend(xTaskMatrizNoturna);
        vTaskSuspend(xTaskDisplayNoturna);

    vTaskStartScheduler();
    panic_unsupported();
}
