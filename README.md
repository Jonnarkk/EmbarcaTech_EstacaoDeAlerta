# Estação de Alerta de Enchentes

## Descrição
O projeto consiste na simulação de uma estação de alerta de enchentes, utilizando a coleta de dados de um sensor de umidade — que, neste caso, foi substituído por um potenciômetro. Com base nos valores lidos, o sistema alterna entre o modo normal e o modo de alerta.

## Funcionalidades
- **LED verde**: Indica que os níveis estão normais.
- **LED vermelho**: Indica que há níveis anormais de volume de chuva ou nível de água.
- **Display**: Mostra mensagens dependendo do modo que o sistema se encontra.
- **Matriz de LED's**: Permanece em cor verde se os níveis estão normais, caso contrário, mostra uma exclamação vermelha para alertar.
- **Buzzer**: Emite sinais sonoros para feedback sonoro.

## Estrutura do Código
O código apresenta diversas funções, das quais vale a pena citar:

- `vJoystickTask()`: Tarefa do FreeRTOS referente à leitura do joystick.
- `vDisplayTask()`: Tarefa do FreeRTOS referente ao acionamento do display.
- `vLedTask()`: Tarefa do FreeRTOS referente ao acionamento do LED RGB.
- `vMatrizTask()`: Tarefa do FreeRTOS referente ao acionamento da matriz de LED's.
- `vBuzzerTask()`: Tarefa do FreeRTOS referente ao acionamento do buzzer.

## Estrutura dos arquivos
```
project/
│
├── lib/
│   ├── font.h
│   ├── ssd1306.c
│   ├── ssd1306.h
│   ├── led_matriz.h
│   ├── led_matriz.c
│   ├── buzzer.h
│   ├── buzzer.c
│
├── DispFilaTasks.c
├── CMakeLists.txt
├── pio_matriz.pio
└── README.md
```
## Desenvolvedor 
Guilherme Miller Gama Cardoso
