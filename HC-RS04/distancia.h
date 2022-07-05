/*
 *  Modulo: distancia.h
 *
 *  Nome: Laura Martin Werneck
 *
 *  Data: 04/07/2022
 *
 *  Descrição: Modulo com as funcoes criadas em  distancia.c
 */

#ifndef DISTANCIA_H_
#define DISTANCIA_H_

#include <msp430.h>
#include <stdint.h>
#include <bits.h>

void init_clock_system(void);
void config_timerB_1();
void trigger();
uint32_t medicao_distancia();

#endif /* DISTANCIA_H_ */
