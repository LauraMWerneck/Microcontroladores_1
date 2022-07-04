/*
 * distancia.c
 *
 *  Created on: 04/07/2022
 *      Author: laura
 */
/*
 * 05_main_timer_input_comparator.c
 *
 *      Exemplo de parcial de aplicação:
 *
 *      - Utiliza o evento de captura do Timer para borda
 *      de subida e descida de uma porta.
 *
 *                MSP430FR2355
 *            -----------------
 *        /|\|              XIN|-
 *         | |                 |
 *         --|RST          XOUT|-
 *           |                 |
 *           |       P2.1/TB1.2| <-- Sensor / Echo1
 *           |             P2.4| <-- Sensor / Trig1
 *           |                 |
 */

#include <msp430.h>
/* Tipos uint16_t, uint8_t, ... */
#include <stdint.h>
#include <bits.h>


#ifndef __MSP430FR2355__
    #error "Example not validated with this device."
#endif


volatile uint16_t distancia = 0;


/* Configura temporizador A */
void config_timerB_1(){
    /* Configura comparador 1 do timer B:
     * CM_3: captura de borda de subida e descida
     * CCIS_0: entrada A
     * CCIE: ativa IRQ
     * CAP: modo captura
     * SCS: captura síncrona
     */
    TB1CCTL1 |= CM_3 | CCIS_0 | CCIE | CAP | SCS;   // -> Mudar para CCR2

    /* Configura timerB1:
     * - TBSSEL_2: SMCLK como clock source
     * - MC_2: modo de contagem contínua
     * - TBCLR: limpa registrador de contagem
     */
    TB1CTL |= TBSSEL_2 | MC_2 | TBCLR;   // -> Mudar para CCR2
}

void trigger(){

    SET_BIT(P2OUT,BIT4);

    __delay_cycles(240);   // Ver o cálculo do numero de ciclos do delay (foto)

    CLR_BIT(P2OUT,BIT4);
}


/* Funcao para retornar o calculo da distancia no main */
uint32_t medicao_distancia(){

    return distancia;
}


/* Timer1 Interrupt Handler
 * Interrupcao para obter o valor do comparador na
 * borda de subbida e na borda de descida*/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER1_B1_VECTOR
__interrupt void TIMER1_B1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_B0_VECTOR))) TIMER1_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{

    /* Variaveis para obter o valor das interrupcoes de borda de subida
     * e descida para a partir desses dados calcular a distancia */
    uint16_t timer_count_0 = 0;
    uint16_t timer_count_1 = 0;

    TB1CCTL0 &= ~CCIFG;

    /* ToDo: validar P2IN para detecção da borda */

    /* Borda de subida */
    if (TST_BIT(P2IN, BIT0)){   // -> Mudar para P2.1
        timer_count_0 = TB1CCR1;   // -> Mudar para CCR2
    }
    /* Borda de descida */
    else {
        timer_count_1 = TB1CCR1;   // -> Mudar para CCR2
        distancia = timer_count_1 - timer_count_0; //calculo da distancia

        /* Acorda main
        __bic_SR_register_on_exit(LPM0_bits); */
    }
}


