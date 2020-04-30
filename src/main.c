#include "boardconfig.h"
#include "controleForno.h"

void app_main()
{
    printf("Inicializando a aplicação... \n");

    /* Abaixo são feitas as chamadas para as funções que executarão
     * todo o processo necessário na inicialização do sistema, como
     * a configuração de GPIOs, setup do ADC, configuração das
     * interrupções externas e inicialização das tasks de controle */
    board_init();
    controle_init();
}
