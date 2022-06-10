@@ -1,148 +1,161 @@
/*
 * Nome: Laura Martin Werneck
 * Data: 09/05/2022
 *
 * Requisitos: Implemente uma aplicação de contagem pulsos para uma industria
 * de alimentos. O sensor de entrada deve ser conectado a um gerador de funções.
 * Um display LCD deve exibir a quantidade pulsos  em tempo real.
 *
 * Descricao: Este codigo configura um display LCD para mostrar a contagem de pulsos
 * de um gerador de função, a partir de sua borda de subida. Para isso ele apresenta
 * em seu display as seguintes informacoes:
 *                         ------------------------------
 *                         | Quant. pulsos:             |
 *                         | Número                     |
 *                         ------------------------------
 * Caso o gerador chegue em 100.000 pulsos, o display mostra que chegou no valor máximo,
 * que foi especificado no projeto.
 * Além de configurar o display tambem foi feito um botao de reset para zerar a contagem
 * a qualquer momento pelo usuário.
 *
 * Os pinos usados para essa implementacao estao representados abaixo segundo o diagrama
 * de pinos:
 *                             MSP430FR2355
 *                       -------------------------
 *                       |                       |
 *         Botao RESET-->|P4.1                   |
 *                       |                       |
 *                       |                   P6.0|--> D4 do LCD
 *                       |                   P6.1|--> D5 do LCD
 *                       |                   P6.2|--> D6 do LCD
 *                  RS-->|P1.3               P6.3|--> D7 do LCD
 *                   E-->|P1.2                   |
 *                       |                  P2.5 |--> Sensor de entrada conectado a um gerador de funções
 *                       -------------------------
 * */

#include <msp430.h> 
#include <stdio.h>

#include "gpio.h"
#include "lcd.h"

#define BUTTON  BIT1           // Porta P4.1 - botao usado como reset
#define SINAL_PORT P2         // Porta P2 como a que recebe o sinal do gerador de funcao
#define SINAL  BIT5         // Porta P2.5 // ///

#define MAX_PULSES 100000      // Definicao do numero maximo de pulsos

volatile uint32_t pulses = 0;  // Inicializa a quantidade de pulsos em zero

void hardware_init()           // Configuracoes do botao de reset
{
    /* Habilita resistor de pull up ou pull down */
    P4REN |= BUTTON;
    /* Habilita resitor como pull up */
    P4OUT |= BUTTON;
}

void config_ext_irq(){            // Configuracoes da interrupcao

    /* Habilitacao da IRQ apenas botao */
    PORT_IE(SINAL_PORT) =  SINAL;

    /* Transicao de nivel baixo para alto - borda de subida */
    /*Como queremos tudo em 0 nao precisa do comando abaixo
     * Port x interrupt edge select
     * 0b = PxIFG flag is set with a low-to-high transition*/
    //PORT_IES(BUTTON_PORT) = BUTTON_0;

    /* Limpa alguma IRQ pendente */
    PORT_IFG(SINAL_PORT) &= ~SINAL;
}


void main(){

    char string[16];

    /* Configuracao de hardware */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    /* Incializa o hardware */
    hardware_init();

    /* Inicializa hardare: veja lcd.h para
     * configurar pinos */
    lcd_init_4bits();
    /* Escreve string */

    /* Configura interup��es */
    config_ext_irq();

    /* Habilita IRQs e desliga CPU */
    __bis_SR_register(GIE);

    lcd_send_data(LCD_LINE_0, LCD_CMD);    // Imprime na primeira linha "Quant. pulsos:"
    lcd_write_string("Quant. pulsos:");

    _delay_cycles(100000);

    while (1){

        lcd_send_data(LCD_LINE_1, LCD_CMD);  // Imprime na segunda linha o valor da quantidade de pulsos
          snprintf(string, 16, "%d", pulses);
        lcd_write_string(string);

        /* Botao 1: reset. Quando pressionado, o valor do contador e zerado.
         * Deteccao na funcao main, sem interrupcao externa.
         * Quando o botao for pressionado manda 0, so que isso nao daria verdadeiro entao por isso o '!'*/
        if (!TST_BIT(P4IN, BUTTON)){ /* Equivalente a: if (P4IN & BIT1) */
            pulses = 0;
            lcd_send_data(LCD_LINE_1, LCD_CMD);
            lcd_write_string("                 ");
        }

        if(pulses >= MAX_PULSES){                  // Se a quant. máxima de pulsos for atingido imprime a mensagem
            lcd_send_data(LCD_LINE_1, LCD_CMD);
            lcd_write_string("Maximo atingido!");
        }

        _delay_cycles(10000);
    }

}

/* Port 1 ISR (interrupt service routine) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
#else
#error Compiler not supported!
#endif
{

    if(pulses < MAX_PULSES)  //Enquanto pulsos for enos que o numero maximo incrementa pulsos
        pulses++;

    /* Limpa sinal de IRQ do bot�o 0 */
    PORT_IFG(SINAL_PORT) &= ~SINAL;
}
