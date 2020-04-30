#include <stdio.h>
#include "definitions.h"
#include "controleForno.h"
#include "boardconfig.h"

static void configPins()
{
    printf("Configurando os pinos de entrada e saída de dados... \n");

    /* Abaixo é feita a configuração das GPIOs onde serão conectados
     * os leds que sinalizam a escolha do modo de funcionamento.
     * Todos eles são configurados como saída*/
    gpio_pad_select_gpio(LED_MODO_ASSAR);
    gpio_set_direction(LED_MODO_ASSAR, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_MODO_GRATINAR);
    gpio_set_direction(LED_MODO_GRATINAR, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_MODO_GRELHAR);
    gpio_set_direction(LED_MODO_GRELHAR, GPIO_MODE_OUTPUT);

    /* Abaixo é feita a configuração da GPIOs que controlará a saída
     * responsável por ligar/desligar a resistência de aquecimento do
     * forno. */
    gpio_pad_select_gpio(PIN_OUTPUT);
    gpio_set_direction(PIN_OUTPUT, GPIO_MODE_OUTPUT);

    /* Abaixo é feita a configuração das GPIOs onde serão conectados
     * os leds que sinalizam a escolha do ponto de cozimento do alimento.
     * Todos eles são configurados como saída*/
    gpio_pad_select_gpio(LED_PONTO_AO_PONTO);
    gpio_set_direction(LED_PONTO_AO_PONTO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_PONTO_BEM_PASSADO);
    gpio_set_direction(LED_PONTO_BEM_PASSADO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_PONTO_MAL_PASSADO);
    gpio_set_direction(LED_PONTO_MAL_PASSADO, GPIO_MODE_OUTPUT);

    /* Abaixo é feita a configuração das GPIOs onde serão conectados
     * os botões que servem para escolher o modo de funcionamento
     * e o ponto de cozimento do forno, além do botão responsável por
     * inicializar uma ação. Todos eles são configurados como entrada. */
    gpio_set_direction(BT_SELECIONA_MODO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BT_SELECIONA_MODO, GPIO_PULLUP_ONLY);

    gpio_set_direction(BT_SELECIONA_PONTO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BT_SELECIONA_PONTO, GPIO_PULLUP_ONLY);

    gpio_set_direction(BT_START, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BT_START, GPIO_PULLUP_ONLY);
}

static void configAdc()
{
    printf("Configurando ADC... \n");

    /* Setup do ADC do ESP32 a ser utilizado */
    adc1_config_width(ADC_WIDTH_12Bit);
    /* Atenuação para leitura de escala total (0 a 3.3v) */
    adc1_config_channel_atten(LM35, ADC_ATTEN_11db);
}

static void configISR()
{
    printf("Configurando interrupções externas... \n");

    /* Abaixo é feita a configuração das interrupções externas. Os botões
     * de escolha modo, ponto e inicialização serão responsáveis por
     * disparar as interrupções. Todas as interrupções foram configuradas
     * para acontecer na borda de descida, e a implementação das funções
     * de callback estão no arquivo controleForno.c */
    gpio_set_intr_type(BT_SELECIONA_MODO, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(BT_SELECIONA_PONTO, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(BT_START, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(BT_SELECIONA_PONTO, bt_ponto_isr_handler, NULL);
    gpio_isr_handler_add(BT_SELECIONA_MODO, bt_modo_isr_handler, NULL);
    gpio_isr_handler_add(BT_START, bt_start_isr_handler, NULL);
}

void board_init()
{
    configPins();
    configAdc();
    configISR();
}