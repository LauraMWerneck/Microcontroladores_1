/*
 * Tarefa 04: medição da tensão das baterias
 *
 * Nome: Laura Martin  Werneck
 *
 * Data: 14 de jun de 2022
 *
 * Descrição: Utilizando o conversor analógico digital, temporizadores e
 * interrupções, implemente a medição e proteção da tensão das baterias
 * de um carrinho autônomo conforme o projeto disponibilizado pelo professor.
 * A tensão de cada célula, são duas baterias em série, não deve ser menor que 3.1V.
 * Exiba os valores da tensão das baterias em dois displays multiplexados.
 *
 * */

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
    uint32_t bateria_1 = 0;
    uint32_t bateria_2 = 0;
    uint16_t primeiro_digito = 0;
    uint16_t segundo_digito = 0;
    uint16_t data = 0;

    /* Para o watchdog timer
     * Necessário para código em depuração */
    WDTCTL = WDTPW | WDTHOLD;

#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    /* Inicializa siatema de clock */
    init_clock_system();

    /* Inicializa displays */
    watchdog_display_mux_init();

    timerB_init();

    init_adc();

    P2DIR = BIT0 | BIT2;

    /* Entra em modo de economia de energia */
    __bis_SR_register(LPM0_bits + GIE);

    while (1){

        bateria_1 = medicao_bateria_1();
        bateria_2 = medicao_bateria_2();

        bateria_1 = bateria_1 - bateria_2;

        if (get_info())  {
            primeiro_digito = bateria_1/10;
            segundo_digito = bateria_1 % 10;

            P2OUT ^= BIT0;
        }
        else {
            primeiro_digito = bateria_2/10;
            segundo_digito = bateria_2 % 10;

            P2OUT ^= BIT2;
        }

        data = segundo_digito;
        data |= primeiro_digito << 4;       //Coloca o primeiro digito deslocado para ficar na primeira "caixinha"
        watchdog_display_mux_write(data);

        /* Desliga CPU até ADC terminar */
        __bis_SR_register(LPM0_bits + GIE);
    }
}


