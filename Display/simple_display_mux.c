/*
 * simple_display_mux.c
 *
 *  Created on: Feb 27, 2020
 *      Author: Renan Augusto Starke
 *      Instituto Federal de Santa Catarina
 */

#include <msp430.h>
#include <stdint.h>

#include "simple_display_mux.h"
#include "bit.h"

/* Tabela de conversão em flash: Anodo comum */
#ifdef COM_ANODO
const uint8_t convTable[] = {0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02,
        0x78, 0x00, 0x18, 0x08, 0x03, 0x46, 0x21, 0x06, 0x0E};
#endif


void display_mux_init() {

    /* Configuração de portas */
    DISPLAYS_DATA_PORT_DIR = 0xff;    //P2DIR_DIR = 0xff;
    DISPLAYS_DATA_PORT_OUT = 0;

    DISPLAYS_MUX_PORT_DIR |= BIT0 | BIT1;    //P1DIR_DIR |= BIT0 | BIT1 | BIT2
}

void display_mux_write(uint8_t data){

    uint8_t d1 = data & 0x0f;
    uint8_t d2 = data >> 4;

    //for (n=NUMBER_DISPLAYS; n > 0; n--){
        /* Desliga todos os displays */
        DISPLAYS_MUX_PORT_OUT = 0x03;    //// binario: 0000 0011

        /* Escreve valor convertido do dígito 1 no GPIO */
        DISPLAYS_DATA_PORT_OUT = convTable[d1];

        /* Liga display 1 */
        CLR_BIT(DISPLAYS_MUX_PORT_OUT, BIT0);

        /* Mantém um tempo ligado:  */
        _delay_cycles(10000);

        /* Desliga display 1 */
        DISPLAYS_MUX_PORT_OUT = 0x03;

        /* Escreve valor convertido do dígito 2 no GPIO */
        DISPLAYS_DATA_PORT_OUT = convTable[d2];

        /* Liga display 2 */
        CLR_BIT(DISPLAYS_MUX_PORT_OUT, BIT1);

        /* Mantém um tempo ligado */
        _delay_cycles(10000);
    //}

}

