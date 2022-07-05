/*
 *  Modulo: distancia.c
 *
 *  Nome: Laura Martin Werneck
 *
 *  Data: 04/07/2022
 *
 *  Descrição: Utilizando um timer em modo captura, configura as funcoes
 *  para medicao da distancia utilizando o sensor HC-SR04.
 *
 *  - Utiliza o evento de captura do Timer para borda
 *    de subida e descida de uma porta.
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


/* Configura temporizador B1.2 */
void config_timerB_1(){
    /* Configura comparador 2 do timer B:
     * CM_3: captura de borda de subida e descida
     * CCIS_0: entrada A
     * CCIE: ativa IRQ
     * CAP: modo captura
     * SCS: captura síncrona
     */
    TB1CCTL2 |= CM_3 | CCIS_1 | CCIE | CAP | SCS;

    /* Configura timer B1.2:
     * - TBSSEL_2: SMCLK como clock source
     * - MC_2: modo de contagem contínua
     * - TBCLR: limpa registrador de contagem
     */
    TB1CTL |= TBSSEL_2 | MC_2 | TBCLR | ID_3;   // Duvida se esta certo ou se tem que por TB1.2...
}

/* Funcao para criar o trigger de acionamento do sensor.
 * Para isso coloca a sida do pino em nivel logico alto
 * por 10us e depois coloca o pino em nivel logico baixo. */
void trigger(){

    /* Coloca a saida do pino P2.4 em nivel logico alto */
    SET_BIT(P2OUT,BIT4);

    /* Petriodo do trigger: Tt = 10*10^-6
     * Periodo da CPU: Tcpu = 1/24*10^6
     * Numero de ciclos = Tt/Tcpu
     * Numero de ciclos = 10*10^-6/(1/24*10^6)
     * Numero de ciclos = 240
     * */
    __delay_cycles(240);

    /* Coloca a saida do pino P2.4 em nivel logico baixo */
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
    if (TST_BIT(P2IN, BIT1)){
        timer_count_0 = TB1CCR2;
    }
    /* Borda de descida */
    else {
        timer_count_1 = TB1CCR2;
        distancia = timer_count_1 - timer_count_0; //calculo da distancia

        /* Acorda main
        __bic_SR_register_on_exit(LPM0_bits); */
    }
}


