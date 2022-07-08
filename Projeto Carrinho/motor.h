/*
 * motor.h
 *
 *  Created on: 06/06/2022
 *      Author: laura
 */

#ifndef MOTOR_H_
#define MOTOR_H_

#define CONTAGEM_MAX_CCR 8000


void inicializa_motores();

void motor_para_frente(uint16_t x);

void motor_para_tras(uint16_t x);

void motor_para_direita(uint16_t x);

void motor_para_esquerda(uint16_t x);

void motor_desligado();

void muda_razao_ciclica();

void muda_sentido();

#endif /* MOTOR_H_ */
