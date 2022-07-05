
/*  Tarefa 05: medição distância HC-SR04
 *
 *  Nome: Laura Martin Werneck
 *
 *  Data: 04/07/2022
 *
 *  Descrição: Utilizando um timer um modo captura, implemente uma aplicação
 *  que estima a distância em relação a objetos através de um sensor HC-SR04.
 *
 * */


#include <msp430.h>
/* Tipos uint16_t, uint8_t, ... */
#include <stdint.h>
#include "bits.h"
#include "gpio.h"
#include "distancia.h"


#ifndef __MSP430FR2355__
    #error "Example not validated with this device."
#endif

/* Pinos de hardware que o sensor esta conectado */
#define SENSOR_PORT P2
#define SENSOR_PIN BIT0



/* Configura sistema de clock para usar o Digitally Controlled Oscillator (DCO) em 24MHz
 * Essa configuração utiliza pinos para cristal externo. */
void init_clock_system(void) {

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
    CSCTL1 = DCORSEL_7;                          // Set DCO = 24MHz
    CSCTL2 = FLLD_0 + 731;                       // DCOCLKDIV = 327358*731 / 1
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



/* Configura temporizador watchdog */
void config_wd_as_timer(){
    /* Configura Watch dog como temporizador:
     *
     * WDT_ADLY_250 <= (WDTPW+WDTTMSEL+WDTCNTCL+WDTIS2+WDTSSEL0+WDTIS0)
     * WDTPW -> "Senha" para alterar confgiuração.
     * WDTTMSEL -> Temporizador ao invés de reset.
     * WDTSSEL -> Fonte de clock de ACLK
     * WDTIS2 -> Período de 250ms com ACLK = 32.768Hz
     */
    WDTCTL = WDT_ADLY_250;
    /* Ativa IRQ do Watchdog */
    SFRIE1 |= WDTIE;
}

void main(void)
{
    /* Para o watchdog timer
     * Necessário para código em depuração */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    /* Configurações de hardware */
    init_clock_system();

    config_timerB_1();
    config_wd_as_timer();


    volatile uint32_t distancia = 0;


    /* Configura o pino P2.4 como saida
     * Pino responsavel por receber o trigger imput
     * para o sensor funcionar */
    P2DIR = BIT4;
    P2OUT = 0;   // Inicializa em nivel logico baixo

    /* Input capture for P2.1 */  // Ver se é assim mesmo
    P2SEL0 = BIT1;
    P2SEL1 = BIT1;

    __bis_SR_register(GIE);

    while(1)
    {
        /* Aciona o trigger para o sensor funcionar */
        trigger();

        /* Entra em modo de economia de energia */
        __bis_SR_register(LPM0_bits + GIE);

       distancia = medicao_distancia();

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
