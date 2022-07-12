/*
 * 08_main_uart.c
 *
 *  Created on: Jun 4, 2020
 *      Author: Renan Augusto Starke
 *      Instituto Federal de Santa Catarina
 *
 *
 *      - Exemplo de recepÃ§Ã£o e transmissÃ£o da USART
 *      - CPU Ã© desligado atÃ© o recebimento dos dados.
 *      - Uma mensagem de ACK Ã© enviado quando um pacote
 *      Ã© recebido.
 *
 *      - Clock da CPU Ã© 24MHZ definido e uart_fr2355.h  devido a
 *      configuraÃ§Ã£o do baudrate.
 *
 *      - VEJA uart_fr2355.c/.h
 *
 *               MSP430FR2355
 *            -----------------
 *        /|\|              XIN|-
 *         | |                 |
 *         --|RST          XOUT|-
 *           |                 |
 *           |    P4.3/UCA1TXD | --> TX
 *           |                 |
 *           |    P4.2/UCA1RXD | <-- RX
 *           |                 |
 */


/* System includes */
#include <msp430.h>
#include <stdint.h>

/* Project includes */
#include "uart_fr2355.h"
#include "bits.h"
#include "gpio.h"
#include "motor.h"
#include "hc_sr04.h"

#ifndef __MSP430FR2355__
#error "Clock system not supported/tested for this device"
#endif



/**
  * Configura sistema de clock para usar o Digitally Controlled Oscillator (DCO) em 24MHz
  * Essa configuraÃ§Ã£o utiliza pinos para cristal externo.
  */
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


int main(){
    //const char message[] = "ACK";
    //const char message_bin_data[] = { 65, 63, 87, 87};

    //char my_data[8];

    /* Desliga Watchdog */
    WDTCTL = WDTPW + WDTHOLD;


#if defined (__MSP430FR2355__)
    /* Disable the GPIO power-on default high-impedance mode */
    PM5CTL0 &= ~LOCKLPM5;
#endif

    /* Inicializa hardware */
    init_clock_system();
    /*Inicializacao da UART*/
    //init_uart();
    /*Inicializacao dos motores*/
    //inicializa_motores();
    /*Inicializacoes do sensor de distancia*/
    config_timerB_1();
    config_wd_as_timer();
    init_sensor();


    volatile uint32_t distancia = 0;

    __bis_SR_register(GIE);

    while (1){


        /* Configura o recebimento de um pacote de 4 bytes */
        //uart_receive_package((uint8_t *)my_data, 1);

        /* Desliga a CPU enquanto pacote nÃ£o chega */
        //__bis_SR_register(CPUOFF | GIE);
/*
        switch (my_data[0]) {
        case 'f':
            motor_para_frente(1000);  //OBS: a velocidade esta inversamente proporcional
            break;
        case 't':
            motor_para_tras(1000);
            break;
        case 'd':
            motor_para_direita(1000);
            break;
        case 'e':
            motor_para_esquerda(1000);
            break;
        case 'o':
            motor_desligado();

            break;

        default:
            break;
        }*/


        /* Envia resposta */
        //uart_send_package((uint8_t *)message, 4);


        /* Aciona o trigger para o sensor funcionar */
        trigger();

        /* Entra em modo de economia de energia */
        __bis_SR_register(LPM0_bits + GIE);

       distancia = medicao_distancia();

        //__bis_SR_register(CPUOFF | GIE);
    }
}



