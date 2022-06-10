/*
 * motor.c
 *
 *  Created on: 06/06/2022
 *  Author: laura
 */

#include <msp430.h>
#include <stdint.h>

#include "motor.h"

#define CONTAGEM_MAX_CCR 8000


void config_timerB_3_as_pwm();

enum {DESLIGADO, FRENTE, TRAS, ESQUERDA, DIREITA};

struct estado_motores{
    uint8_t direcao;
    uint16_t velocidade;
};

volatile struct estado_motores estado_carrinho = {DESLIGADO, 0};


/*
 * Configura temporizador B3 com contagem up e down.
 */
void config_timerB_3_as_pwm(){

    /* Estamos usando TB3CCR0 para contagem máxima
     * que permite controle preciso sobre o período
     * é possível usar o overflow */

    /* Configuração dos comparadores como PWM:
     *
     * TB3CCR0: Timer3_B Capture/Compare 0: período do PWM
     *
     * OUTMOD_2: PWM output mode: 2 - PWM toggle/reset
     *
     * TB3CCR1 PWM duty cycle: TB3CCR1 / TB3CCR0 *
     * TA2CCR1 PWM duty cycle: TA2CCR1 / TA1CCR0 */

    TB3CCR0 = CONTAGEM_MAX_CCR-1;


    /*      .
     *      /|\                  +                < -Comparador 0: (máximo da contagem) -> período do PWM
     *       |                 +   +
     *       |               +       +
     *       |-------------+---------- +          <--  Comparadores 1 e 2: razão cíclica
     *       |           +  |         | +
     *       |         +    |         |   +
     *       |       +      |         |     +
     *       |     +        |         |       +
     *       |   +          |         |         +
     *       | +            |         |           +
     * Timer +--------------|---- ----|-------------->
     *       |              |
     *       |
     *
     *       |--------------+         |--------------
     * Saída |              |         |
     *       +---------------++++++++++------------->
     */

    /* TBSSEL_2 -> Timer B clock source select: 2 - SMCLK
     * MC_1     -> Timer B mode control: 1 - Up to CCR0
     * ID_3     ->  Timer B input divider: 3 - /8
     *
     * Configuração da fonte do clock do timer 1 */
    TB3CTL = TBSSEL_2 | MC_3 | ID_0;
}

void inicializa_motores(){

    config_timerB_3_as_pwm();


    /* Ligação físicas do timer nas portas */
    /* TB3.1 é o P6.0
     * TB3.2 é o P6.1
     *
     * P6.0 e P6.1 geram mesmo sinal PWM
     *
     * TB3.3 é o P6.2
     * TB3.4 é o P6.3
     *
     * P6.2 e P6.3 geram mesmo sinal PWM
     *
     * */
    P6DIR = BIT0 | BIT1 | BIT2 | BIT3;

    P6OUT = 0;

    /* Função alternativa: ligação dos pinos no temporizador
     *
     * P6.0 -> TB3.1
     * P6.1 -> TB3.2
     * P6.2 -> TB3.3
     * P6.3 -> TB3.4
     * */
    P6SEL0 = BIT0 | BIT1 | BIT2 | BIT3;

}

void motor_para_frente(uint16_t x){

    TB3CCTL1 = OUTMOD_6;
    TB3CCTL2 = OUTMOD_0;
    TB3CCTL3 = OUTMOD_6;
    TB3CCTL4 = OUTMOD_0;

    TB3CCR1 = x;
    TB3CCR3 = x;

    estado_carrinho.direcao = FRENTE;
    estado_carrinho.velocidade = x;

}

void motor_para_tras(uint16_t x){

    TB3CCTL1 = OUTMOD_0;
    TB3CCTL2 = OUTMOD_6;
    TB3CCTL3 = OUTMOD_0;
    TB3CCTL4 = OUTMOD_6;

    TB3CCR2 = x;
    TB3CCR4 = x;

    estado_carrinho.direcao = TRAS;
    estado_carrinho.velocidade = x;

}

void motor_para_direita(uint16_t x){

    TB3CCTL1 = OUTMOD_6;
    TB3CCTL2 = OUTMOD_0;
    TB3CCTL3 = OUTMOD_0;
    TB3CCTL4 = OUTMOD_6;

    TB3CCR1 = x;
    TB3CCR4 = x;

    estado_carrinho.direcao = DIREITA;
    estado_carrinho.velocidade = x;

}


void motor_para_esquerda(uint16_t x){

    TB3CCTL1 = OUTMOD_0;
    TB3CCTL2 = OUTMOD_6;
    TB3CCTL3 = OUTMOD_6;
    TB3CCTL4 = OUTMOD_0;

    TB3CCR2 = x;
    TB3CCR3 = x;

    estado_carrinho.direcao = ESQUERDA;
    estado_carrinho.velocidade = x;

}

/* Muda a razao ciclica para + 10% do valor máximo (8000)*/
void muda_razao_ciclica(){

    estado_carrinho.velocidade = estado_carrinho.velocidade - 800;

    if (estado_carrinho.velocidade > CONTAGEM_MAX_CCR){
        estado_carrinho.velocidade = 6000;
    }

    switch(estado_carrinho.direcao){
    case FRENTE:
        motor_para_frente(estado_carrinho.velocidade);
        break;
    case TRAS:
        motor_para_tras(estado_carrinho.velocidade);
        break;
    case DIREITA:
        motor_para_direita(estado_carrinho.velocidade);
        break;
    case ESQUERDA:
        motor_para_esquerda(estado_carrinho.velocidade);
        break;
    default:
        break;
    }
}


void muda_sentido(){

    switch (estado_carrinho.direcao) {
    case DESLIGADO:
        motor_para_frente(4000);
        break;
    case FRENTE:
        motor_para_tras(4000);
        break;
    case TRAS:
        motor_para_direita(4000);
        break;
    case DIREITA:
        motor_para_esquerda(4000);
        break;
    case ESQUERDA:
        estado_carrinho.direcao = DESLIGADO;

        TB3CCTL1 = OUTMOD_0;
        TB3CCTL2 = OUTMOD_0;
        TB3CCTL3 = OUTMOD_0;
        TB3CCTL4 = OUTMOD_0;

        break;

    default:
        break;
    }


}

