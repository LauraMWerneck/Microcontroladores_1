/*
 * Nome: Laura Martin Werneck
 * Data: 24/05/2022
 *
 * Descricao: Implementar um programa utilizando MSP430 responsavel por gerar as
 * formas de onda (D0 a D2) conforme a figura:
 *
 *
 *
 *      Exemplo de configuração Timer Tipo B número 0
 *
 *
 *       .
 *      /|\                  +    <-- Overflow (TB0IV_TBIFG - TIMER0_B1_VECTOR)   (2^16)
 *       |                 +
 *       |               +
 *       |             +  <-- Comparador 0 (TBCCR0  -> TIMER0_B0_VECTOR)
 *       |           +
 *       |         +
 *       |       + <--- Comparadores 1 e 2 (TBCCR1 e TBCCR2 - TIMER0_B1_VECTOR)
 *       |     +
 *       |   +
 *       | +
 *       +----------------------------------------->
 *
 *       - Comparadores podem ser configurados para qualquer valor
 *       entre 0 e 65535. IRQs devem ser habilitadas individuais
 *       nos respectivos registradores.
 *
 */

#ifndef __MSP430FR2355__
    #error "Clock system not supported for this device"
#endif

/* System includes */
#include <msp430.h>

/* Project includes */
#include "gpio.h"
#include "bit.h"


#define PINOS P4
#define D0 BIT6
#define D1 BIT7
#define D2 BIT4


/**
  * @brief  Configura sistema de clock para usar o Digitally Controlled Oscillator (DCO).
  *         Utililiza-se as calibrações internas gravadas na flash.
  *         Exemplo baseado na documentação da Texas: msp430g2xxx3_dco_calib.c
  *         Configura ACLK para utilizar VLO = ~10KHz
  * @param  none
  *
  * @retval none
  */
void init_clock_system(void) {             //Inicia o clock da CPU com frequencia de 16MHz

    // Configure two FRAM wait state as required by the device data sheet for MCLK
    // operation at 24MHz(beyond 8MHz) _before_ configuring the clock system.
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
    CSCTL1 = DCORSEL_5;                          // Set DCO = 16MHz
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

 /*
  * Configura temporizador B.
  */
void config_timerB_3(){
    /* Timer B3:
     *
     * TBSSEL_2 ->             Clock de SMCLK.
     * MC_2 -> Contagem crescente.
     * TBIE -> Habilitação de IRQ.
     * ID_1 -> Prescaler = /2
     */
    TB3CTL = TBSSEL_2 | MC_2 | ID_1 | TBIE;

    /* IRQ por comparação entre contagem e comparador 0 */
    TB3CCTL0 = CCIE;
    /* Valor de comparação é 16384   -> 25% de 2^16 */
    TB3CCR0 = 16384;

    /* IRQ por comparação entre contagem e comparador 1 */
    TB3CCTL1 = CCIE;
    /* Valor de comparação é 26214   -> 40% de 2^16 */
    TB3CCR1 = 26214;

    /* IRQ por comparação entre contagem e comparador 2 */
    TB3CCTL2 = CCIE;
    /* Valor de comparação é 49152   -> 75% de 2^16 */
    TB3CCR2 = 49152;

    /* IRQ por comparação entre contagem e comparador 3 */
    TB3CCTL3 = CCIE;
    /* Valor de comparação é 58982   -> 90% de 2^16 */
    TB3CCR3 = 58982;

}


int main(void)
{
    /* Desliga watchdog imediatamente */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    /* Configurações de hardware */
    init_clock_system();
    config_timerB_3();


    PORT_DIR(PINOS) = D0 | D1  | D2;   //Configura todos os pinos como saida
    PORT_OUT(PINOS) = 0;               // Zera todas as saidas (desliga tudo)

    /* Entra em modo de economia de energia com IRQs habilitadas */
    __bis_SR_register(LPM0_bits + GIE);
}




////////////////////////////////////////////////////////////


/* ISR0 do Timer B: executado no evento de comparação  comparador 0 (TACCR0) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER3_B0_VECTOR
__interrupt void TIMER3_B0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER3_B0_VECTOR))) Timer_B (void)
#else
#error Compiler not supported!
#endif
{

    SET_BIT(PORT_OUT(PINOS), D1);  //Comparação 0 (Inicio D1) - D1 em nível logico alto
}



/* ISR1 do Timer B: executado toda a vez que o temporizador estoura, evento do comparador 1 ou evento do comparador 2 */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER3_B1_VECTOR
__interrupt void TIMER3_B1_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER3_B1_VECTOR))) TIMER3_B1_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(TB3IV,TBxIV_TBIFG))
    {
    /* Vector  0:  No interrupt */
    case  TBxIV_NONE:
        break;


    /* Vector  2:  TACCR1 CCIFG -> Comparação 1 (Fim D1)*/
    case  TBxIV_TBCCR1:
        CLR_BIT(PORT_OUT(PINOS), D1);   //D1 em nível logico baixo
        break;

    /* Vector  4:  TACCR2 CCIFG -> Comparação 2 (Inicio D2)*/
    case TBxIV_TBCCR2:
        SET_BIT(PORT_OUT(PINOS), D2);   //D2 em nível logico alto
        break;

    /* Vector  6:  TACCR3 CCIFG -> Comparação 3 (Fim D2)*/
    case TBxIV_TBCCR3:
        CLR_BIT(PORT_OUT(PINOS), D2);    //D2 em nível logico baixo
        break;

     /* Vector 10:  TAIFG -> Overflow do timer */
    case TBxIV_TBIFG:
        PORT_OUT(PINOS) ^= D0;
        break;
    default:
        break;
    }
}
