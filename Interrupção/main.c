/*
 * 03_main_ext_irq.c
 *
 *  Created on: Mar 12, 2020
 *      Author: Renan Augusto Starke
 *      Instituto Federal de Santa Catarina
 */

#include <msp430.h>

#define LED_0   BIT0
#define LED_1   BIT6

#define BUTTON_0  BIT1
#define BUTTON_1  BIT3

void config_ext_irq(){
    /* Primeiramente configura porta:
     *
     * Cuidado quando há entrada e saídas
     *
     * Todos pinos como entrada  */
    P4DIR = 0x00;
    P2DIR = 0x00;

    /* Pull up/down */
    P4REN = BUTTON_0;
    P2REN = BUTTON_1;

    /* Pull up */
    P4OUT = BUTTON_0;
    P2OUT = BUTTON_1;

    /* Habilitação da IRQ apenas botão */
    P4IE =  BUTTON_0;
    P2IE =  BUTTON_1;

    /* Transição de nível alto para baixo */
    P4IES = BUTTON_0;
    P2IES = BUTTON_1;

    /* Limpa alguma IRQ pendente */
    P4IFG &= ~BUTTON_0;
    P2IFG &= ~BUTTON_1;
}

void main(){
    /* Configuração de hardware */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    /* Configura port do LED */
    P1DIR = LED_0;
    P6DIR = LED_1;


    /* Configura interupções */
    config_ext_irq();

    /* Habilita IRQs e desliga CPU */
    __bis_SR_register(LPM4_bits | GIE);

}


/* Port 4 ISR (interrupt service routine) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_4 (void)
#else
#error Compiler not supported!
#endif
{
    /* Liga/desliga LED quando detectado borda no botão */
    P1OUT ^= LED_0;

    /* Limpa sinal de IRQ do botão 0 */
    P4IFG &= ~BUTTON_0;
}

/* Port 2 ISR (interrupt service routine) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT6_VECTOR))) Port_2 (void)
#else
#error Compiler not supported!
#endif
{
    /* Liga/desliga LED quando detectado borda no botão */
    P6OUT ^= LED_1;

    /* Limpa sinal de IRQ do botão 0 */
    P2IFG &= ~BUTTON_1;
}
