/*
 * regs.h
 *
 *  Created on: Jan 22, 2023
 *      Author: Darrin
 */

#ifndef REGS_H_
#define REGS_H_
#include <stdint.h>

typedef struct {
	uint32_t MODER;
	uint32_t OTYPER;
	uint32_t OSPEEDER;
	uint32_t PUPDR;
	uint32_t IDR;
	uint32_t ODR;
	uint32_t BSRR;
	uint32_t LCKR;
	uint32_t AFRL;
	uint32_t AFRH;
} GPIO;


typedef struct {
	uint32_t CR;
	uint32_t PLLCFGR;
	uint32_t CFGR;
	uint32_t CIR;
	uint32_t AHB1RSTR;
	uint32_t AHB2RSTR;
	uint32_t AHB3RSTR;
	uint32_t rsvd0;
	uint32_t APB1RSTR;
	uint32_t APB2RSTR;
	uint32_t rsvd1;
	uint32_t rsvd2;
	uint32_t AHB1ENR;
	uint32_t AHB2ENR;
	uint32_t AHB3ENR;
	uint32_t rsvd3;
	uint32_t APB1ENR;
	uint32_t APB2ENR;
	uint32_t rsvd4;
	uint32_t rsvd5;
	uint32_t AHB1LPENR;
	uint32_t AHB2LPENR;
	uint32_t AHB3LPENR;
	uint32_t rsvd6;
	uint32_t APB1LPENR;
	uint32_t APB2LPENR;
	uint32_t rsvd7;
	uint32_t rsvd8;
	uint32_t BDCR;
	uint32_t CSR;
	uint32_t rsvd9;
	uint32_t rsvd10;
	uint32_t SSCGR;
	uint32_t PLLI2SCFGR;
	uint32_t PLLISAICFGR;
	uint32_t DCKCFGR;
	uint32_t CKGATENR;
	uint32_t DCKCFGR2;
} RCCr;

typedef struct {
	uint32_t CR1;
	uint32_t CR2;
	uint32_t SMCR;
	uint32_t DIER;
	uint32_t SR;
	uint32_t EGR;
	uint32_t CCMR1;
	uint32_t CCMR2;
	uint32_t CCER;
	uint32_t CNT;
	uint32_t PSC;
	uint32_t ARR;
	uint32_t rsvd0;
	uint32_t CCR1;
	uint32_t CCR2;
	uint32_t CCR3;
	uint32_t CCR4;
	uint32_t rsvd1;
	uint32_t DCR;
	uint32_t OR;
} TIMx;

typedef struct {
	uint32_t CRTL;
	uint32_t LOAD;
	uint32_t VAL;
	uint32_t CALIB;
} SYSTick;




#endif /* REGS_H_ */
