/*
 *  Modulo: distancia.c
 *
 *  Nome: Laura Martin Werneck
 *
 *  Data: 04/07/2022
 *
 *  DescriÃ§Ã£o: Utilizando um timer em modo captura, configura as funcoes
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
#include "bits.h"
#include "gpio.h"
#include "hc_sr04.h"


#ifndef __MSP430FR2355__
#error "Example not validated with this device."
#endif

#define LED2_PORT P6
#define LED_2 BIT6


volatile uint16_t distancia = 0;

/* Configura temporizador watchdog */
void config_wd_as_timer(){
    /* Configura Watch dog como temporizador:
     *
     * WDT_ADLY_250 <= (WDTPW+WDTTMSEL+WDTCNTCL+WDTIS2+WDTSSEL0+WDTIS0)
     * WDTPW -> "Senha" para alterar confgiuraÃ§Ã£o.
     * WDTTMSEL -> Temporizador ao invÃ©s de reset.
     * WDTSSEL -> Fonte de clock de ACLK
     * WDTIS2 -> Perioodo de 250ms com ACLK = 32.768Hz
     */
    WDTCTL = WDT_ADLY_16;
    /* Ativa IRQ do Watchdog */
    SFRIE1 |= WDTIE;
}

/* Configura temporizador B1.2 */
void config_timerB_1(){
    /* Configura comparador 2 do timer B:
     * CM_3: captura de borda de subida e descida
     * CCIS_0: entrada A
     * CCIE: ativa IRQ
     * CAP: modo captura
     * SCS: captura sÃ­ncrona
     */
    TB1CCTL2 |= CM_3 | CCIS_0 | CCIE | CAP | SCS;

    /* Configura timer B1.2:
     * TBSSEL_2: SMCLK como clock source
     * MC_2: modo de contagem contÃ­nua
     * TBCLR: limpa registrador de contagem
     * TBIE -> Habilitação de IRQ.
     */
    TB1CTL |= TBSSEL_2 | MC_2 | TBCLR | ID_3;
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

    /* Fazer o timer resetar quando acontecer o trigger*/
   TB1CTL |= TBCLR;

}

void init_sensor(){

    /* Configura o pino P2.4 como saida
     * Pino responsavel por receber o trigger imput
     * para o sensor funcionar */
    P2DIR = BIT4;
    P2OUT = 0;   // Inicializa em nivel logico baixo

    /* Input capture for P2.1 */
    P2SEL0 = BIT1;

    PORT_DIR(LED2_PORT) = LED_2;
    PORT_OUT(LED2_PORT) = 0;
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



    /*usar para gerar mais de uma atividade na interrupÃ§Ã£o
     * ver exemplo 05_main_simple_timer0_b.c*/
    switch(__even_in_range(TB1IV,TBxIV_TBIFG)){

    /* Vector  0:  No interrupt */
    case  TBxIV_NONE:
        break;

        /* Vector  2:  TACCR1 CCIFG -> ComparaÃ§Ã£o 1*/
    case  TBxIV_TBCCR1:

        break;
        /* Vector  4:  TACCR2 CCIFG -> ComparaÃ§Ã£o 2*/
    case TBxIV_TBCCR2:

        TB1CCTL2 &= ~CCIFG;

        PORT_OUT(LED2_PORT) ^= LED_2;

        if (TST_BIT(P2IN, BIT1)){
            timer_count_0 = TB1CCR2;
        }
        /* Borda de descida */
        else {
            timer_count_1 = TB1CCR2;
            distancia = timer_count_1 - timer_count_0;  //calculo da distancia

            /* Acorda main
                __bic_SR_register_on_exit(LPM0_bits); */
        }
        break;

        /* Vector 10:  TAIFG -> Overflow do timer */
    case TBxIV_TBIFG:
        //PORT_OUT(LED2_PORT) ^= LED_2;
        break;
    default:
        break;
    }

}


/* ISR do watchdog: executado toda a vez que o temporizador estoura.
 * Usado para acordar o main periodicamente para poder dar o pulso do trigger. */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
    /*Acorda o main*/
    __bic_SR_register_on_exit(LPM0_bits);
}
