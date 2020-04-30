#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "driver/adc.h"

/* Definições gerais:                                           */
/* Flag default usada para instalação das interrupções externas:*/
#define ESP_INTR_FLAG_DEFAULT       0
/* Número de amostras na filtragem de medidas do ADC:           */
#define NUMBER_OF_SAMPLES           40
/* Tempo para cada modo de funcionamento em °C:                 */
#define TEMPERATURA_ASSAR           145
#define TEMPERATURA_GRATINAR        275
#define TEMPERATURA_GRELHAR         265
/* Tempo para cada ponto em ms: */
#define TEMPO_MAL_PASSADO           20000
#define TEMPO_AO_PONTO              30000
#define TEMPO_BEM_PASSADO           40000

/* Definições de tipos: */
typedef enum {ASSAR = 0, GRATINAR, GRELHAR} modo_t;
typedef enum {MAL_PASSADO = 0, AO_PONTO, BEM_PASSADO} ponto_t;
typedef enum {AGUARDANDO_ACAO = 0, ACAO_INICIADA} status_t;

/* Definições das GPIOs que serão utilizadas no projeto */
/* GPIO dos botões:                                     */
#define BT_SELECIONA_MODO       32
#define BT_SELECIONA_PONTO      33
#define BT_START                25
/* GPIO dos LEDS indicativos:                           */
#define LED_MODO_GRELHAR        12
#define LED_MODO_ASSAR          13
#define LED_MODO_GRATINAR       14
#define LED_PONTO_MAL_PASSADO   5
#define LED_PONTO_AO_PONTO      18
#define LED_PONTO_BEM_PASSADO   19
/* GPIO onde o LM35será conectado:                      */
#define LM35                    ADC1_CHANNEL_7
/* GPIO Pino de saída que controlará a resistência      */
#define PIN_OUTPUT              2

#endif /* DEFINITIONS_H */