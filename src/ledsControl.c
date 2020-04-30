
#include "ledsControl.h"

static void setLedsModo(uint32_t ledAssarLevel,
                            uint32_t ledGratinarLevel,
                            uint32_t ledGrelharLevel)
{
    gpio_set_level(LED_MODO_ASSAR, ledAssarLevel);
    gpio_set_level(LED_MODO_GRATINAR, ledGratinarLevel);
    gpio_set_level(LED_MODO_GRELHAR, ledGrelharLevel);
}

static void setLedsPonto(uint32_t ledAoPontoLevel,
                            uint32_t ledBemPassadoLevel,
                            uint32_t ledMalPassadoLevel)
{
    gpio_set_level(LED_PONTO_AO_PONTO, ledAoPontoLevel);
    gpio_set_level(LED_PONTO_BEM_PASSADO, ledBemPassadoLevel);
    gpio_set_level(LED_PONTO_MAL_PASSADO, ledMalPassadoLevel);
}

void updateLedsModo(modo_t modo)
{
    switch (modo)
    {
    case ASSAR:
        setLedsModo(1, 0, 0);
        break;
    case GRATINAR:
        setLedsModo(0, 1, 0);
        break;
    case GRELHAR:
        setLedsModo(0, 0, 1);
        break;
    default:
        setLedsModo(0, 0, 0);
        break;
    }
}

void updateLedsPonto(ponto_t ponto)
{
    switch (ponto)
    {
    case MAL_PASSADO:
        setLedsPonto(0, 0, 1);
        break;
    case AO_PONTO:
        setLedsPonto(1, 0, 0);
        break;
    case BEM_PASSADO:
        setLedsPonto(0, 1, 0);
        break;
    default:
        setLedsPonto(0, 0, 0);
        break;
    }
}