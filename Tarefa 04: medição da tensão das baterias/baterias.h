/*
 * baterias.h
 *
 *  Created on: 14 de jun de 2022
 *      Author: Aluno
 */

#ifndef BATERIAS_H_
#define BATERIAS_H_

void timerB_init();

void init_adc();

uint32_t medicao_bateria_1();
uint32_t medicao_bateria_2();
uint8_t get_info();


#endif /* BATERIAS_H_ */
