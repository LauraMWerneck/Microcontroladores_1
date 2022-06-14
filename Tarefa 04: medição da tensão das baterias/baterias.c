/*
 * baterias.c
 *
 *  Created on: 14 de jun de 2022
 *      Author: Aluno
 *
 *
 * 06_main_adc_isr_timer_fr2355.c
 *
 *  Created on: Oct 02, 2020
 *      Renan Augusto Starke
 *      Instituto Federal de Santa Catarina
 *
 *      Exemplo do conversor analógico digital.
 *      - Trigger do ADC pelo timer B1.1
 *      - Sequẽncia de canais A0->A1->A2
 *      - ISR do timer desnecessário pois usa-se o hardware
 *      do timer para iniciar uma nova conversão
 *
 *                  MSP430FR2355
 *               -----------------
 *           /|\|              XIN|-
 *            | |                 |
 *            --|RST          XOUT|-
 *              |                 |
 *  LED    <--  | P1.6    P1.0 A0 | <--
 *              |         P1.0 A1 | <--
 *              |         P1.0 A2 | <--
 *              |                 |
 */

#include <msp430.h>

/* Tipos uint16_t, uint8_t, ... */
#include <stdint.h>
#include <stdio.h>

#ifndef __MSP430FR2355__
#error "Clock system not supported for this device"
#endif

#include "gpio.h"
#include "bits.h"
#include "watchdog_display_mux.h"
#include "baterias.h"

volatile uint16_t adc_data[3] = {0};   //Vetor de 16 bits sem sinal


/* Configura temporizador B1 para trigger do ADC */
void timerB_init(){

    /* Contagem máxima do timer 1 */
    TB1CCR0 = 200-1;                   // Da a frequencia de amostragem
    /* Habilita saída interna do do comparador 0: CCR1 reset/set */
    TB1CCTL1 = OUTMOD_7;
    /* Valor de comparação 1: deve ser menor que TB1CCR0 */
    TB1CCR1 = 100;                     // Onde acontece o triger do timer
    /* Configura timerB1:
    * - TBSSEL_2: SMCLK como clock source
    * - MC_2: modo de contagem contínua
    * - TBCLR: limpa registrador de contagem
    */
    TB1CTL = TBSSEL_2 | MC_2 | TBCLR;
}


void init_adc(){

    /* Configura pinos P1.0 a P1.2 como entrada do AD */
    P1SEL0 |=  BIT0 + BIT1 + BIT2;           // Configuracoes conforme a tabela do MSP430
    P1SEL1 |=  BIT0 + BIT1 + BIT2;

    /* 16ADCclks, ADC ON */
    ADCCTL0 |= ADCSHT_2 | ADCON;   // Liga o ADC
    /* ADC clock MODCLK, sampling timer, TB1.1B trig.,repeat sequence */
    ADCCTL1 |= ADCSHP | ADCSHS_2 | ADCCONSEQ_2;
    /* 8-bit conversion results */
    ADCCTL2 &= ~ADCRES;
    /* 12-bits conversion results */
    ADCCTL2 |= ADCRES_2;                    //Configura como sendo de 12 bits

    /* A0~2(EoS); Vref=1.5V */
    ADCMCTL0 |= ADCINCH_0 | ADCSREF_0;
    /* Enable ADC ISQ */
    ADCIE |= ADCIE0;

    /* Configure reference interna  */
    PMMCTL0_H = PMMPW_H;                                        // Unlock the PMM registers
    PMMCTL2 |= INTREFEN;                                        // Enable internal reference
    __delay_cycles(400);                                        // Delay for reference settling

    /* Enable ADC */
    ADCCTL0 |= ADCENC;
    /* Limpar timer para maior sincronismo */
    /* TB1CTL |= TBCLR;  */
}

uint32_t medicao_baterias(){

    volatile uint32_t tensao_bateria = 0;

    //Inicio parte do codigo do AD
    /* Debug LED */
    P6DIR |= BIT6;

    /* Calculo da tensão da bateria
     * ADC = Vin*2¹²/Vref
     * Vin = ADC*3,3*10/2¹²
     * tensao_bateria = (ADC*3,3*10)/2¹²
     * Tem o uint32_t antes do adc para tranformar ele em um número de 32 bits. */
    tensao_bateria = ((uint32_t)adc_data[0]*33) >> 12;

    return tensao_bateria;

}


// ADC interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC_VECTOR))) ADC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    static uint8_t i = 0;

    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:
            break;
        case ADCIV_ADCTOVIFG:
            break;
        case ADCIV_ADCHIIFG:
            break;
        case ADCIV_ADCLOIFG:
            break;
        case ADCIV_ADCINIFG:
            break;
        case ADCIV_ADCIFG:

            /* Obter amostras */
            adc_data[i] = ADCMEM0;

            if(i == 0)
                i = 2;
            else
                i--;

            P6OUT ^= BIT6;
            __bic_SR_register_on_exit(LPM0_bits + GIE);
            break;
        default:
            break;
    }
}


/* Timer1 Interrupt Handler */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER1_B1_VECTOR
__interrupt void TIMER1_B1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_B0_VECTOR))) TIMER1_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{


}

