#include "controleForno.h"
#include "ledsControl.h"
#include "definitions.h"
#include "esp_log.h"

/* Comente a linha abaixo para desativar os logs de debug */
#define DEBUG 1

/* A struct _action servirá para moldar como será o funcionamento
 * do forno durante o cozimento através do conjunto de variáveis
 * contidas nela. Um exemplo é, o forno deverá assar (modo_t ASSAR)
 * uma carne ao ponto (ponto_t AO_PONTO). A variáveis status serve
 * para controle do fluxo, registrando se o forno está assando ou
 * aguardando uma nova ação. */
typedef struct _action {
    modo_t modo;
    ponto_t ponto;
    status_t status;  
} action_t;

/* Declaração de variáveis action que moldará uma ação de cozimento
 * de alimentos no forno. */
static action_t action;

/* Declaração do handler de cada Task */
static TaskHandle_t xSelecionaModoHandle;
static TaskHandle_t xSelecionaPontoHandle;
static TaskHandle_t xStartHandle;
static TaskHandle_t xAdcReadHandle;
static TaskHandle_t xOutputControlHandle;

/* Declaração do handle da Queue usada para trocar mensagens entre a
 * task que faz aquisição de valores do sensor analógico LM35, e a task
 * que usa esses valores para controlar a saída */
xQueueHandle adc_queue;

/* Declaração do dandler do timer que contará o tempo que a resistência
 * deverá ficar ligada em função do ponto escolhido pelo operador. */
TimerHandle_t xTempoDeFuncionamentoHandle;

/* Implementação da função de callback para o tratamento da interrupção
 * externa do botão de seleção do modo. */
void IRAM_ATTR bt_modo_isr_handler( void * pvParameter)
{
    /* Deve-se executar a task que trata a interrupção Somente se o status for
     * aguardando ação, pois se o status for ACAO_INICIADA, significa que o
     * forno já começou a execução de alguma operação, e não é possível escolher
     * outro modo de funcionamento até que a ação em execução seja finalizada */
    if(action.status == AGUARDANDO_ACAO)
    {
        if(xSelecionaModoHandle != NULL)
        {
            /* Se o botão foi pressionado, a task que trata a ação é notificada
             * pela chamada a seguir. */
            vTaskNotifyGiveFromISR(xSelecionaModoHandle, pdFALSE);
        }
    }
}

/* Implementação da função de callback para o tratamento da interrupção
 * externa do botão de seleção do ponto. */
void IRAM_ATTR bt_ponto_isr_handler( void * pvParameter)
{
    /* Deve-se executar a task que trata a interrupção Somente se o status for
     * aguardando ação, pois se o status for ACAO_INICIADA, significa que o
     * forno já começou a execução de alguma operação, e não é possível escolher
     * outro modo de funcionamento até que a ação em execução seja finalizada */
    if(action.status == AGUARDANDO_ACAO)
    {
        /* Se o botão foi pressionado, a task que trata a ação é notificada
         * pela chamada a seguir. */
        vTaskNotifyGiveFromISR(xSelecionaPontoHandle, pdFALSE);
    }
}

/* Implementação da função de callback para o tratamento da interrupção
 * externa do botão de inicialização de uma ação. */
void IRAM_ATTR bt_start_isr_handler( void * pvParameter)
{
    /* Deve-se executar a task que trata a interrupção Somente se o status for
     * aguardando ação, pois se o status for ACAO_INICIADA, significa que o
     * forno já começou a execução de alguma operação, e não é possível escolher
     * outro modo de funcionamento até que a ação em execução seja finalizada */
    if(action.status == AGUARDANDO_ACAO)
    {
        /* Se o botão foi pressionado, a task que trata a ação é notificada
         * pela chamada a seguir. */
        vTaskNotifyGiveFromISR(xStartHandle, pdFALSE);
    }
}

/* O forno deverá atingir uma temperatura de acordo com o modo de funcionamento
 * escolhido pelo operador (definições de valores no arquivo definitions.h),
 * desta forma, a função auxiliar getTemperaturaAlvoDoModo foi criada para retornar
 * o valor definido de temperatura em função do modo passado para ela como parâmetro,
 * ou seja, ela recebe o modo e retorna um valor de temperatura em graus Celsius. */
static uint32_t getTemperaturaAlvoDoModo(modo_t modo)
{
    switch (modo)
    {
    case ASSAR:
        return TEMPERATURA_ASSAR;
        break;
    case GRATINAR:
        return TEMPERATURA_GRATINAR;
        break;
    case GRELHAR:
        return TEMPERATURA_GRELHAR;
        break;
    default:
        return -1;
        break;
    }
}

/* O forno deverá fazer a contagem do tempo de cozimento para controlar o ponto
 * do alimento que está sendo preparado. Ou seja, o tempo de um cozimento é função
 * do ponto escolhido pelo operador (definições de valores no arquivo definitions.h),
 * desta forma, a função auxiliar getTempoDeFuncionamentoDoPonto foi criada para
 * retornar o valor definido de tempo em função do ponto passado para ela como
 * parâmetro, ou seja, ela recebe o ponto e retorna um valor de tempo em
 * mili segundos. */
static uint32_t getTempoDeFuncionamentoDoPonto(ponto_t ponto)
{
    switch (ponto)
    {
    case MAL_PASSADO:
        return TEMPO_MAL_PASSADO;
        break;
    case AO_PONTO:
        return TEMPO_AO_PONTO;
        break;
    case BEM_PASSADO:
        return TEMPO_BEM_PASSADO;
        break;
    default:
        return -1;
        break;
    }
}

/* Task usada para fazer a seleção do modo de funcionamento, que pode
 * variar entre ASSAR, GRATINAR e GRELHAR */
void selecionaModo(void *pvParameter)
{
    /* Variável de controle da máquina de estados de seleção de modos.
     * Estado inicial ASSAR */
    uint32_t estado = ASSAR;

    while(true)
    {
        #ifdef DEBUG
            ESP_LOGI("Task selecionaModo", "Modo selecionado: %d\n", action.modo); 
        #endif
        /* Caso o botão seja pressionado e o status seja aguardando ação,
         * o callback da interrupção do botão de seleção do modo irá
         * notificar esta task e então a condição abaixo será satisfeita */
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
        {
            /* O switch case abaixo faz a transição entre os modos a cada vez
             * que o botão de seleção de modo é pressionado. Dentro do case de
             * cada modo a variável estado registra o valor do próximo estado
             * para fazer a transição entre modos quando o botão for novamente
             * pressionado, e o valor do modo escolhido é salvo em action.modo. */
            switch (estado)
            {
            case ASSAR:
                estado = GRATINAR;
                action.modo = ASSAR;
                break;
            case GRATINAR:
                estado = GRELHAR;
                action.modo = GRATINAR;
                break;
            case GRELHAR:
                estado = ASSAR;
                action.modo = GRELHAR;
                break;
            default:
                break;
            }
            /* Os leds indicativos de modo são atualizados de acordo com o valor que foi
             * recém selecionado */
            updateLedsModo(action.modo);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task usada para fazer a seleção do ponto de cozimento, que pode
 * variar entre MAL_PASSADO, AO_PONTO e BEM_PASSADO */
void selecionaPonto(void *pvParameter)
{
    /* Variável de controle da máquina de estados de seleção de pontos.
     * Estado inicial MAL_PASSADO */
    uint32_t estado = MAL_PASSADO;

    while(true)
    {
        #ifdef DEBUG
            ESP_LOGI("Task selecionaPonto", "Ponto selecionado: %d\n", action.ponto); 
        #endif
        /* Caso o botão seja pressionado e o status seja aguardando ação,
         * o callback da interrupção do botão de seleção do modo irá
         * notificar esta task e então a condição abaixo será satisfeita */
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
        {
            /* O switch case abaixo faz a transição entre os pontos a cada vez
             * que o botão de seleção de ponto é pressionado. Dentro do case de
             * cada ponto a variável estado registra o valor do próximo estado
             * para fazer a transição entre pontos quando o botão for novamente
             * pressionado, e o valor do ponto escolhido é salvo em action.ponto */
            switch (estado)
            {
            case MAL_PASSADO:
                estado = AO_PONTO;
                action.ponto = MAL_PASSADO;
                break;
            case AO_PONTO:
                estado = BEM_PASSADO;
                action.ponto =  AO_PONTO;
                break;
            case BEM_PASSADO:
                estado = MAL_PASSADO;
                action.ponto = BEM_PASSADO;
                break;
            default:
                break;
            }
            /* Os leds indicativos de ponto são atualizados de acordo com o valor que foi
             * recém selecionado */
            updateLedsPonto(action.ponto);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task usada para inicializar uma ação */
void start(void *pvParameter)
{
    /* Inicio do loop infinito da task start */
    while(true)
    {
        /* Caso o botão start seja pressionado e o status seja aguardando ação,
         * o callback da interrupção do botão irá notificar esta task e então
         * a condição abaixo será satisfeita */
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
        {
            /* O status deve ser mudado para indicar que uma ação foi iniciada.
             * Isso fará o travamento da seleção de modo, ponto e do próprio start,
             * pois as interrupções não irão notificar as tasks no período em que
             * o alimento estiver sendo preparado. */
            action.status = ACAO_INICIADA;

            /* A chamada abaixo dispara o timer de acordo com o tempo definido em função
             * do ponto de cozimento do alimento */
            xTimerChangePeriod(xTempoDeFuncionamentoHandle,
                                pdMS_TO_TICKS(getTempoDeFuncionamentoDoPonto(action.ponto)),
                                0);

            /* A temperatura do forno deverá ser controlada para obedecer ao modo de 
             * funcionamento, desta forma as tasks adcRead e OutputControl deverão
             * estar em executaçâo, pois ela tem esse papel. */
            vTaskResume(xAdcReadHandle);
            vTaskResume(xOutputControlHandle);

            /* Já que no período em que a ação estiver sendo executada, nada mais além
             * do controle da saída e da leitura da temperatura poderá ser feito,
             * então as tasks que não serão utilizadas durante este período serão
             * suspendidas */
            vTaskSuspend(xSelecionaModoHandle);
            vTaskSuspend(xSelecionaPontoHandle);

            #ifdef DEBUG
                ESP_LOGI("Task start", "Modo %d selecionado. A temperatura alvo e de %d graus Celsius",
                            action.modo, getTemperaturaAlvoDoModo(action.modo));
                ESP_LOGI("Task start", "Ponto %d selecionado. O tempo de cozimento sera de %d milisegundos",
                            action.ponto, getTempoDeFuncionamentoDoPonto(action.ponto));
                ESP_LOGI("Task start", "Status %d", action.status); 
            #endif
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task que faz a leitura do valor de tensão da saída do sensor de
 * temperatura e o salva na  fila adc_queue para que a task OutputControl
 * consuma esses dados. */
void adcRead(void *pvParameters )
{
    uint32_t adc_raw_read = 0;
    uint32_t filter = 0;
    int i = 0;

    while(1)
    {
        filter = 0;

        /* O laço de repetição abaixo é usado para se realizar uma especie de
         * filtro de médias na leitura do sensor para minimizar o número elevado
         * de pequenas variações que o sensor apresenta. Apenas o valor da media
         * de NUMBER_OF_SAMPLES (definitions.h) é salvo na fila*/
        for(i = 0 ; i < NUMBER_OF_SAMPLES ; i++)
        {
            adc_raw_read = adc1_get_raw(LM35);
            filter += adc_raw_read;
        }
        adc_raw_read = filter/NUMBER_OF_SAMPLES;

        /* Adiciona o valor na fila */
        xQueueSend(adc_queue, &adc_raw_read, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Task que consome os valores da fila que contem os dados da leitura do sensor
 * e controla a saída de acordo com a temperatura alvo. */
void OutputControl(void *pvParameters )
{
    uint32_t recv_value = 0;
    uint32_t temperaturaAtual = 0;
    while(1)
    {
        /* Retirando dado da fila e atribuindo o valor para a variável recv_value */
        xQueueReceive(adc_queue, &recv_value, portMAX_DELAY);

        /* O cálculo abaixo faz a conversão do valor digital para graus Celsius.
         * O sensor apresenta uma tensão saída de 10mV/°C, desta forma descobre-se
         * o valor de temperatura equivalente da seguinte forma:
         * 1 - Divida o valor da escala total de tensão (0 a 3.3) pelo 
         *     número total de níveis possíveis (2^12) = 3.3/4095
         * 2 - Multiplique o valor de divisão anterior pelo número da leitura digital
         *     contido em recv_value e terá o valor da tensão lida = (recv_value*3.3/4095)
         * 3 - Para descobrir o valor em grauas celsius, divida o resultado do passo 2
         *     pelo valor de 10mV, que é a relação entre tensão e temperatura do
         *     sensor = (recv_value*3.3/4095)/0.010 */
        temperaturaAtual = ((recv_value * 3.3) / 4095) / 0.010;
        #ifdef DEBUG
            ESP_LOGI("OutputControl", "ADC temperature read from LM35: %d graus celsius", temperaturaAtual); 
        #endif

        /* O controle de temperatura é feito ligando/desligando a saída que ativa
         * a resistência que aquecerá o forno. Quando a temperatura passa do valor,
         * a resistência é desligada, quando ela volta ao normal a resistência é
         * religada. */
        if (temperaturaAtual > getTemperaturaAlvoDoModo(action.modo))
        {
            gpio_set_level(PIN_OUTPUT, 0);
        }
        else
        {
            gpio_set_level(PIN_OUTPUT, 1);
        }
    }
}

/* Após a contagem do tempo de funcionamento, o timer chamará este callback,
 * e ela é responsável por voltar o sistema ao estado inicial */
void callBackTimer(TimerHandle_t pxTimer)
{
    gpio_set_level(PIN_OUTPUT, 0);          /* Desliga a resistência                                            */
    action.status = AGUARDANDO_ACAO;        /* Volta para o estado AGUARDANDO_ACAO                              */
    vTaskResume(xSelecionaModoHandle);      /* Inicializa novamente a task de leitura de modo                   */
    vTaskResume(xSelecionaPontoHandle);     /* Inicializa novamente a task de leitura do ponto                  */
    vTaskSuspend(xAdcReadHandle);           /* Suspende a task que faz a leitura do sensor de temperatura       */
    vTaskSuspend(xOutputControlHandle);     /* Suspende a task que faz o controle da temperatura da resistencia */
}

void controle_init()
{
    printf("Inicializando as tasks de controle do forno...\n");

    /* Inicialização de variaveis */
    action.status = AGUARDANDO_ACAO;
    action.ponto = MAL_PASSADO;
    action.modo = ASSAR;
    updateLedsModo(action.modo);
    updateLedsPonto(action.ponto);

    /* Criação do timer que contará o tempo de cozimento*/
    xTempoDeFuncionamentoHandle = xTimerCreate("Tempo de funcionamento", pdMS_TO_TICKS(1000), pdFALSE, 0, callBackTimer);
    if(xTempoDeFuncionamentoHandle == NULL)
    {
        #ifdef DEBUG
            ESP_LOGE("controle_init", "Erro na inicialização do timer de controle"); 
        #endif
        return;
    }

    /*  Instanciação da Queue que será usada para troca de informações entre a task que fará a leitura
     *  e conversão A/D da tensão do sensor e a task que controlará a saída */
    adc_queue = xQueueCreate(20, sizeof(int));
    if(adc_queue == NULL)
    {
        #ifdef DEBUG
            ESP_LOGE("controle_init", "Erro na inicialização da fila do ADC"); 
        #endif
        return;
    }

    /* Criação de todas as tasks: */
    xTaskCreate(&selecionaModo, "Seleciona Modo", 2048, NULL, 0, &xSelecionaModoHandle);
    xTaskCreate(&selecionaPonto, "Seleciona Ponto", 2048, NULL, 0, &xSelecionaPontoHandle);
    xTaskCreate(&start, "Inicializa processo", 2048, NULL, 0, &xStartHandle);
    xTaskCreate(&adcRead, "Leitura ADC", 2048, NULL, 0, &xAdcReadHandle);
    xTaskCreate(&OutputControl, "Controle da saida", 2048, NULL, 0, &xOutputControlHandle);

    if( xSelecionaModoHandle == NULL  ||
        xSelecionaPontoHandle == NULL ||
        xStartHandle == NULL          ||
        xAdcReadHandle  == NULL       ||
        xOutputControlHandle == NULL)
    {
        #ifdef DEBUG
            ESP_LOGE("controle_init", "Erro na inicialização das tasks"); 
        #endif
        return;
    }

    /* As tasks adcRead e OutputControl só funcionarão quando o botão start
     * for pressionado e uma ação estiver sendo executada */
    vTaskSuspend(xAdcReadHandle);
    vTaskSuspend(xOutputControlHandle);
}