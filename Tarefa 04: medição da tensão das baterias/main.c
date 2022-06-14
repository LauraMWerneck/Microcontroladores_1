/*
 * 02_simple_display_mux.c
 *
 *  Created on: Feb 27, 2020
 *      Author: Renan Augusto Starke
 *      Instituto Federal de Santa Catarina
 *
 *      Exemplo de aplicação:
 *
 *      - Utiliza o WatchDog Timer
 *      para multiplexar 2 displays de 7 segmentos.
 *      - Utiliza IRQ externa para contar número
 *      de pulsos exibindo-os no display.
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

// Botao para interrupcao do display
#define BUTTON  BIT1
#define BUTTON_PORT P4
#define PULSES  BIT1

// i é usado para incrementar o valor do display
volatile uint16_t i = 0;


// Configuração de IRQ para os botões usados com os displays
void config_ext_irq(){
    /* Pull up/down */
    P4REN = PULSES;

    /* Pull up */
    P4OUT = PULSES;

    /* Habilitação da IRQ apenas botão */
    P4IE =  PULSES;

    /* Transição de nível alto para baixo */
    P4IES = PULSES;

    /* Limpa alguma IRQ pendente */
    P4IFG &= ~PULSES;
}


/**   Configura sistema de clock para usar o Digitally Controlled Oscillator (DCO).
  *   Utililiza-se as calibrações internas gravadas na flash.
  *   Exemplo baseado na documentação da Texas: msp430g2xxx3_dco_calib.c
  *   Configura ACLK para utilizar VLO = ~10KHz
  */
void init_clock_system(void) {             //Inicia o clock da CPU com frequencia de 16MHz

    // Configure two FRAM wait state as required by the device data sheet for MCLK
    // operation at 16MHz(beyond 24MHz) _before_ configuring the clock system.
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


void main(void)
{
    /* Para o watchdog timer
     * Necessário para código em depuração */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    /* Inicializa siatema de clock */
    init_clock_system();

    /* Inicializa configuracao da IRQ */
    config_ext_irq();

    /* Inicializa displays */
    watchdog_display_mux_init();

    timerB_init();

    init_adc();

    uint32_t bateria_1 = 0;

    uint16_t primeiro_digito = 0;

    uint16_t segundo_digito = 0;

    uint16_t data = 0;

    /* Entra em modo de economia de energia */
    __bis_SR_register(LPM0_bits + GIE);

    while (1){

       bateria_1 = medicao_baterias();

       primeiro_digito = bateria_1/10;

       segundo_digito = bateria_1 % 10;

       data = segundo_digito;

       data |= primeiro_digito << 4;

       watchdog_display_mux_write(data);

        /* Desliga CPU até ADC terminar */
        __bis_SR_register(LPM0_bits + GIE);
    }
}

/* Port 1 ISR (interrupt service routine)
 * Interrupcao de quando detecta borda de sibida no
 * botao e ai avanca um numero no display. */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT4_VECTOR))) Port_4 (void)
#else
#error Compiler not supported!
#endif
{
    /* Liga/desliga LED quando detectado borda no botão */
    watchdog_display_mux_write(i++);

    /* Limpa sinal de IRQ do botão 0 */
    P4IFG &= ~PULSES;
}
