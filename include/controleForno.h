#ifndef CONTROLEFORNO_H
#define CONTROLEFORNO_H

/* Inclusão de elementos-chave do FreeRTOS: */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "esp_log.h"

extern void controle_init();
extern void IRAM_ATTR bt_modo_isr_handler( void * pvParameter);
extern void IRAM_ATTR bt_ponto_isr_handler( void * pvParameter);
extern void IRAM_ATTR bt_start_isr_handler( void * pvParameter);

#endif /* CONTROLEFORNO_H */