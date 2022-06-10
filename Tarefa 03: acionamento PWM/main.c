/*
 * 05_main_simple_pwm.c
 *
 *  Created on: Apr 22, 2020
 *      Author: Renan Augusto Starke
 *      Instituto Federal de Santa Catarina
 *
 *      Exemplo de parcial de aplicação:
 *
 *      - Gera multiplos PWMs com o temporizador B3
 */

#include <msp430.h>

/* Tipos uint16_t, uint8_t, ... */
#include <stdint.h>

#include "gpio.h"
#include "bits.h"

#include "motor.h"


#define BUTTON_PORT_1 P4
#define BUTTON_1 BIT1
#define BUTTON_PORT_2 P2
#define BUTTON_2 BIT3


#define BUTTON_SAMPLES (12)


void init_clock_system(void) {             //Inicia o clock da CPU com frequencia de 16MHz

    // Configure two FRAM wait state as required by the device data sheet for MCLK
    // operation at 16MHz(beyond 8MHz) _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_2 ;

    P2SEL1 |= BIT6 | BIT7;                       // P2.6~P2.7: crystal pins
    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);           // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);                   // Test oscillator fault flag

    __bis_SR_register(SCG0);                     // disable FLL
    CSCTL3 |= SELREF__XT1CLK;                    // Set XT1 as FLL reference source
    CSCTL0 = 0;                                  // clear DCO and MOD registers
    CSCTL1 = DCORSEL_5;                          // Set DCO = 8MHz
    CSCTL2 = FLLD_0 + 489;                       // DCOCLKDIV = 32735*489 / 1
    __delay_cycles(3);
    __bic_SR_register(SCG0);                     // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));   // FLL locked

    /* CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;
     * set XT1 (~32768Hz) as ACLK source, ACLK = 32768Hz
     * default DCOCLKDIV as MCLK and SMCLK source
     - Selects the ACLK source.
     * 00b = XT1CLK with divider (must be no more than 40 kHz)
     * 01b = REFO (internal 32-kHz clock source)
     * 10b = VLO (internal 10-kHz clock source) (1)   */
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;
}


void config_timerB_0(){
    /* Timer B0:
     * TASSEL_2 -> Clock de SMCLK.
     * MC_1 -> Contagem crescente até CCR0.
     * ID_2 -> Prescaler = /4
     */
    TB0CTL = TBSSEL_2 | MC_1 | ID_2;

    /* IRQ por comparação entre contagem e comparador 0 */
    TB0CCTL0 = CCIE;
    /* Valor de comparação é 40000 */
    TB0CCR0 = 40000;
}


void main(void)
{
    volatile uint16_t my_data = 0;

    /* Para o watchdog timer
     * Necessário para código em depuração */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif


    /* Configura botões */
    /* BUTTON_PORT totalmente como entrada */
    PORT_DIR(BUTTON_PORT_1) = 0;
    PORT_DIR(BUTTON_PORT_2) = 0;
    /* Resistores de pull up */
    PORT_REN(BUTTON_PORT_1) = BUTTON_1;
    PORT_OUT(BUTTON_PORT_1) = BUTTON_1;
    PORT_REN(BUTTON_PORT_2) = BUTTON_2;
    PORT_OUT(BUTTON_PORT_2) = BUTTON_2;

    /* Configurações de hardware */
    init_clock_system();
    config_timerB_0();
    inicializa_motores();


    __bis_SR_register(LPM0_bits + GIE);

    while(1)
    {
        //__bis_SR_register(LPM0_bits + GIE);

        /* Código de baixa prioridade da aplicação */
    }
}


/* ISR0 do Timer B: executado no evento de comparação  comparador 0 (TBCCR0)
 *
 * Utilizado para o debouncer por amostragem: faz a verificação de botão
 * periodicamente.
 *
 * */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) Timer_B (void)
#else
#error Compiler not supported!
#endif
{
    static uint8_t counter_b1 = BUTTON_SAMPLES;
    static uint8_t counter_b2 = BUTTON_SAMPLES;

    /* Se botão 1 apertado: borda de descida */
    if (!TST_BIT(PORT_IN(BUTTON_PORT_1), BUTTON_1))  {
        /* Se contagem = 0, debounce terminado */
        if (!(--counter_b1)) {

            muda_razao_ciclica();
        }
    }
    else
        counter_b1 = BUTTON_SAMPLES;

    if (!TST_BIT(PORT_IN(BUTTON_PORT_2), BUTTON_2))  {
        /* Se contagem = 0, debounce terminado */
        if (!(--counter_b2)) {

            muda_sentido();

        }
    }
    else
        counter_b2 = BUTTON_SAMPLES;
}


