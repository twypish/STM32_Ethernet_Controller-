/*
 * transmitter.c
 *
 *  Created on: Jan 31, 2024
 *      Author: Hampel Matthew
 */

#include <stdio.h>
#include <string.h>
#include "transmitter.h"
#include "monitor.h"
#include "regs.h"
#include <stdlib.h>


#define sys_address 0xE000E010
#define half_ms 8000
#define MS 160000
#define IDLE 5
#define BUSY 6
#define COLLISION 7
#define RETRANSMISSION 8

static volatile GPIO* const GPIOB = (GPIO*)0x40020400;

//set up SysTick Timer
static volatile SYSTick* const systick = (SYSTick*)0xE000E010;
static volatile RCCr* const RCC = (RCCr*)0x40023800;
static volatile uint32_t* const NVIC_ISER = (uint32_t*)0xE000E100;
static volatile TIMx* const TIM4 = (TIMx*)0x40000800;
static volatile TIMx* const TIM3 = (TIMx*)0x40000400;
static volatile int current_bit = 0;
static volatile int trans_message[888];
static volatile int max_size = 0;

static volatile int current_output = 1;

static volatile int current_half = 0;
static volatile int retransmit = 0;
static volatile int failed = 0;

void init_transmitter(){

	// make sure the systick's off
	systick->CTRL = (0<<0);


	//set pin A13 for transmitting

	//set pin A13 for output
	GPIOB->MODER &= ~(0b11<<6);
	GPIOB->MODER |= (0b01<<6);

	//turn on Interrupt Enable
	systick->CTRL |= (1<<1) |(1<<2);

	GPIOB->ODR |= (1<<3);

	//load the systick with 0.5 ms
	systick->LOAD = (half_ms -1);

	// set up tim4 interrupt
	RCC->APB1ENR |= 1<<2;
	NVIC_ISER[0] = 1<<30;
	// output compare mode
	TIM4->CCMR1 &= ~(0b11);
	// toggle on match
	TIM4->CCMR1 = 0b11 << 4;
	// output
	TIM4->CCER = 0;
	TIM4->DIER |= (1<<0);
	// prescaler so we can count up to 1s
	TIM4->PSC = MS;

}
void transmit(char* message, int address, int length){

	//clear previous transmission message
	clear_trans_message();
	clear_buffer();
	set_state(BUSY);
	failed = 0;
	retransmit = 0;
	//Place message into trans_message
	int count = 0;

	// adding preamble in (U = 0x55)
	char character = 'U';
	for (int i = 0; i < 8; i++){
		if (((character >> (7-i)) & 1) == 1){
				trans_message[count++] = 0;
				trans_message[count++] = 1;
		} else {
				trans_message[count++] = 1;
				trans_message[count++] = 0;
		}
	}
	// adding sender address
	int sending_address = 0x3C;
	for (int i = 0; i < 8; i++){
		if (((sending_address >> (7-i)) & 1) == 1){
				trans_message[count++] = 0;
				trans_message[count++] = 1;
		} else {
				trans_message[count++] = 1;
				trans_message[count++] = 0;
		}
	}
	// adding receive address
	for (int i = 0; i < 8; i++){
		if (((address >> (7-i)) & 1) == 1){
				trans_message[count++] = 0;
				trans_message[count++] = 1;
		} else {
				trans_message[count++] = 1;
				trans_message[count++] = 0;
		}
	}

	// adding the length
	int str_length = length;
	for (int i = 0; i < 8; i++){
		if (((str_length >> (7-i)) & 1) == 1){
				trans_message[count++] = 0;
				trans_message[count++] = 1;
		} else {
				trans_message[count++] = 1;
				trans_message[count++] = 0;
		}
	}

	// adding CRC flag
	int flag = 0x00;
	for (int i = 0; i < 8; i++){
		if (((flag >> (7-i)) & 1) == 1){
				trans_message[count++] = 0;
				trans_message[count++] = 1;
		} else {
				trans_message[count++] = 1;
				trans_message[count++] = 0;
		}
	}

	// adding in message
	for (int i = 0; i < length; i++){
		character = message[i];
		//convert character to binary
		for (int j = 0; j < 8; j++){
			if (((character >> (7-j)) & 1) == 1){
				trans_message[count++] = 0;
				trans_message[count++] = 1;
			} else {
				trans_message[count++] = 1;
				trans_message[count++] = 0;
			}
		}
	}
	// adding CRC field 0xAA = 170
	int CRC_field = 170;
	for (int i = 0; i < 8; i++){
		if (((CRC_field >> (7-i)) & 1) == 1){
				trans_message[count++] = 0;
				trans_message[count++] = 1;
		} else {
				trans_message[count++] = 1;
				trans_message[count++] = 0;
		}
		}

	max_size = count;
	systick->CTRL |= (3);
}
// clears the trans_message array
void clear_trans_message(){
	for (int i = 0; i < 888; i++){
		trans_message[i] = 0;
	}
}

void SysTick_Handler(){
	//disable interrupt
	systick->CTRL &= ~(1<<1|1<<0);
	//check that there's still a part of a message to transmit
	if (max_size > 0){
		//If message is fully transmitted
		if (current_bit == max_size){
			max_size = 0;
			current_bit = 0;
			current_output = 1;
			set_state(IDLE);
			GPIOB->ODR |= (1<<3);
		} else {
			int state = get_state();
			if (state != COLLISION){
				current_output = trans_message[current_bit];
				if(current_bit < max_size){
					current_bit++;
				}
				systick->CTRL |= (3);
				if (current_output == 1){
					GPIOB->ODR |= (1<<3);
				} else {
					GPIOB->ODR &= ~(1<<3);
				}
			} else {
				// if in collision, random back off
				// must wait till out of collision to start timer
				// calc random value that is between 1 and Nmax (200)
				// r* 5ms
				int r = rand() % 200;
				TIM4->ARR = (r*5);
				TIM4->CCR1 = (r*5);
				set_state(RETRANSMISSION);
			}
		}

	} else {
		set_state(IDLE);
		retransmit = 0;
	}

}


void TIM4_IRQHandler(){
	// clear flag
	TIM4->SR = 0;
	// turn on isr
	TIM4->DIER &= ~(1<<1);
	TIM4->DIER &= ~(1);
	// turn off counter
	TIM4->CR1 =0;
	// check if idle
	if(retransmit < 15){
		int state = get_state();
		if(state== IDLE){
			// turn on systick and restart transmission (up to 15 tries)
			current_bit = 0;
			systick->CTRL |= (3);
		} else {
			// if not in idle, wait again
			if(state != COLLISION){
				// wait again
				TIM4->CR1 = 1;
			} else {
				set_state(RETRANSMISSION);
			}
			TIM4->DIER |= (1);
		}
		retransmit++;
	} else {
		// give up
		failed = 1;
	}
}

int get_failed_status(){
	return failed;
}
void set_failed_status(int fail){
	failed = fail;
}
