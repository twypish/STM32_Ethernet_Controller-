/*
 * monitor.c
 *
 *  Created on: Jan 23, 2024
 *      Author: wypishinski-prechter
 */

#include "monitor.h"
#include "regs.h"


#define rcc_ahb1enr 0x40023830
#define GPIOBEN (1<<1)
#define GPIOAEN (1<<0)
#define gpiob_moder 0x40020400
#define gpiob_odr 0x40020414
#define moder_clear (0xFF3FFC00)
#define moder_setoutput (0x55155400)
#define all_lights (0xF7E0)
#define bottom_bits (0x3F)
#define top_bits (0x3C0)
#define IDLE 5
#define BUSY 6
#define COLLISION 7
#define max_16_bit_val 65535

static volatile RCCr* const RCC = (RCCr*)0x40023800;

static volatile TIMx* const TIM2 = (TIMx*)0x40000000;
static volatile TIMx* const TIM3 = (TIMx*)0x40000400;

static volatile uint32_t* const NVIC_ISER = (uint32_t*)0xE000E100;

static volatile GPIO* const GPIOA = (GPIO*)0x40020000;

static int state = 0;
static int pin15 = 0;
static int prev_pin = -1;
static int prev_state = -1;
static uint32_t prev_time = 0;
static buffer buff;

static int bits_read = 0;
static int raw_data[16];

static int buff_full = 3;
static int bad_pre = 2;
static int not_for_us = 0;
static int valid = 1;
static int time;
static int const our_ip_top = 0x3F;
static int const our_ip_bot = 0x3C;
static int max = 255;

void init_leds(){
	// enable clock to GPIOB peripheral
	uint32_t *ahb1enr = (uint32_t*)rcc_ahb1enr;
	*ahb1enr |= GPIOBEN;
	// set PB5 - PB15 to output
	uint32_t *moder = (uint32_t*)gpiob_moder;
	// clear the current PB15-PB15 moder bits
	*moder &= ~moder_clear;
	// set the moder bits to output mode
	*moder |= moder_setoutput;

}


void init_timers(){
	// clock on for TIM3 and TIM2, bit 1 in APB1ENR
	RCC->APB1ENR |= 1<<1 | 1<<0;

	// Will be using interrupts, need to enable in NVIC TIM3
	NVIC_ISER[0] = 1<<29;

	// start timer
	TIM2->CR1 = 1;

	//Set timer to max
	TIM2->ARR = 0xFFFFFFFF;

	// filters signal
	TIM2->CCMR1 &= ~(0b11<<4);
	TIM2->CCMR1 |= (0b11<<4);


	// makes Tim2 read only
	TIM2->CCMR1 |= 0b01;


	// Connect timer channel to input pin
	TIM2->CCER &= ~(1<<3) | ~(1<<1);
	TIM2->CCER |= (0b1011);

	//start interrupt
	TIM2->DIER = (1<<1);

	// Will be using interrupts, need to enable in NVIC TIM2
	NVIC_ISER[0] = 1<<28;

	//Set status register to 0
	//TIM2->SR = 0;

	//TIM3->PSC = 94914;

	// Toggle on match mode
	TIM3->CCMR1 = 0b11 << 4;

	// Connect timer channel to output pin
	TIM3->CCER = 0;

	// turn on interrupt for UIE
	TIM3->DIER |= 1;

	int capture_time = TIM3->CNT;
	//set TIM3 CCR1 and ARR here
	TIM3->ARR = 18112;
	TIM3->CCR1 = 1 + capture_time;
	TIM3->CR1  = 1;
}


void TIM2_IRQHandler() {
	// clear isr flag
	TIM2->SR = 0;

	TIM2->DIER &= ~(1<<1);
	// turn off tim3
	TIM3->CR1 = 0;
	TIM3->CNT = 0;

	//turn on timer3 interrupt
	TIM3->DIER |= 1;

	// read current state
	// set A15 to input- rmw
	GPIOA->MODER &= ~(0b11<<30);
	//read pin: initial capture of wave
	pin15 = (GPIOA->IDR >> 15) & 1;

	// change to alt function mode
	GPIOA->MODER &= ~(0b11<<30);
	GPIOA->MODER |= (0b10<<30);
	GPIOA->AFRH &= ~(0b1111<<28);
	GPIOA->AFRH |= (0b01<<28);

	// set state to busy, edge is found
	state = BUSY;
	// write number to PB5 - PB15 (skipping PB11)
	uint32_t *odr = (uint32_t*)gpiob_odr;
	// clear the LED lights
	*odr = *odr & ~all_lights;
	// Or odr's value with a 1 shifted to the left by number
	*odr |= (1<<state);
	//*odr |= (1<<8);

	//*odr = *odr & ~all_lights;

	uint32_t new_time = TIM2->CCR1;

	if(buff.init == -1){
		buff.size = 0;
		buff.init  = 1;
		buff.valid = 1;
		buff.pre = -1;
		buff.sor = -1;
		buff.dest = -1;
		buff.len = -1;
		buff.crc = -1;
		buff.trail = -1;

		raw_data[0] = 1;
		raw_data[1] = 0;
		bits_read = 2;
		prev_pin = pin15;
		prev_time = new_time;
		prev_state = 1;
	}

	time = 0;
	if (new_time >= prev_time) {
	       time = new_time - prev_time;
	   } else {
	        time = (0xFFFFFFFF - prev_time) + new_time;
	   }
	    prev_time = new_time;

	//500 us +- 1.32%
	if(buff.valid == valid){
		// short time between edges
		if((7800 < time) && (time < 9000)){
			raw_data[bits_read] = pin15;
			bits_read++;
			prev_pin = pin15;
			//Long time between edge
		} else if((15500 < time) && (time < 17500)){
			raw_data[bits_read] = pin15;
			bits_read++;
			raw_data[bits_read] = prev_pin;
			bits_read++;
			prev_pin = pin15;
		}
	}

	if(bits_read == 16){
		int ascii_conv = 0;
		for(int i = 1; i < 16; i+=2){
			ascii_conv |= raw_data[i];

			//skips the last shift
			if(i < 15){
				ascii_conv = ascii_conv << 1;
			}
		}


		if(buff.pre == 1 && buff.size != max){
			//check source
			if(buff.sor == -1){
				buff.sor = ascii_conv;
			}else {
				//check destination
				if (buff.dest  == -1){
					buff.dest = ascii_conv;
				}else if(our_ip_bot < buff.dest && buff.dest > our_ip_top){
					buff.valid = not_for_us;
				} else {
					if(buff.len == -1){
						if(ascii_conv > 0){
							buff.len = ascii_conv;
						}else{
							buff.valid = bad_pre;
						}
					} else{
						if(buff.crc == -1){
							if(ascii_conv == 1 || ascii_conv == 0){
								buff.crc = ascii_conv;
							}else{
								buff.valid = bad_pre;
							}
						}else{
							if(buff.size <= buff.len){
								buff.ascii_buff[buff.size] = ascii_conv;
								buff.size++;
							}else{
								if(buff.crc == 0){
									if(buff.trail == -1 && ascii_conv == 0xAA){
										buff.trail = ascii_conv;
									} //else{
										//buff.valid = bad_pre;
									//}
								} else{
									buff.valid = bad_pre;
								}
							}
						}
					}
				}
			}
		} else if (buff.size == max){
			buff.valid = buff_full;
		}else if(buff.pre == -1 && ascii_conv != 0x55){
			buff.ascii_buff[buff.size++] = (char)ascii_conv;
			buff.valid = bad_pre;
			buff.pre = 0;
		} else if (buff.pre == -1 && ascii_conv == 0x55){
			buff.pre = 1;
		}
		bits_read = 0;
	}

	TIM2->DIER |= (1<<1);

	// turn on timer
	TIM3->CR1 = 1;
	TIM2->CR1 = 1;
}


void TIM3_IRQHandler(){
	//set CCER = 1
	// clear isr flag
	TIM3->SR &= 0;

	// turn off both isrs
	TIM3->DIER &= ~(1);
	TIM2->DIER &= ~(1<<1);

	if(buff.init != -1){
		prev_state = -1;
		buff.init = -1;
		}

	// turn off timer
    //	TIM2->CR1 = 0;
	// check pin for high or low
	if(pin15 == 1){
		state = IDLE;
	} else{
		state = COLLISION;
	}
	// write number to PB5 - PB15 (skipping PB11)
	uint32_t *odr = (uint32_t*)gpiob_odr;
	// clear the LED lights
	*odr = *odr & ~all_lights;
	// Or odr's value with a 1 shifted to the left by number
	*odr |= (1<<state);
	// turn on timer2 and interrupt
//	TIM2->CR1 = 1;
	TIM2->DIER |= (1<<1);

}

void init_receivepin(){
	//using pin A15 as our recieve pin (works with tim2)
	// enable clock to GPIOA peripheral
	uint32_t *ahb1enr = (uint32_t*)rcc_ahb1enr;
	*ahb1enr |= GPIOAEN;
	// set A15 to input- rmw
	GPIOA->MODER &= ~(0b11<<30 );

	//read pin: initial capture of wave
	pin15 = (GPIOA->IDR >> 15) & 1;
	state = BUSY;
	// write number to PB5 - PB15 (skipping PB11)
	uint32_t *odr = (uint32_t*)gpiob_odr;
	// clear the LED lights
	*odr = *odr & ~all_lights;
	// Or odr's value with a 1 shifted to the left by number
	*odr |= (1<<state);
	// change to alt function mode
	GPIOA->MODER &= ~(0b11<<30);
	GPIOA->MODER |= (0b10<<30);
	GPIOA->AFRH &= ~(0b1111<<28);
	GPIOA->AFRH |= (0b01<<28);
}

int get_state(){
	return state;
}

void set_state(int new_state){
	state = new_state;
	uint32_t *odr = (uint32_t*)gpiob_odr;
	// clear the LED lights
	*odr = *odr & ~all_lights;
	// Or odr's value with a 1 shifted to the left by number
	*odr |= (1<<state);
}

buffer get_buffer(){
	return buff;
}

void clear_buffer(){
	buff.size = -1;
	buff.valid = 1;
	buff.pre = -1;
	for (int i = 0; i < 100; i++){
		buff.ascii_buff[i] = 0;
	}
}


