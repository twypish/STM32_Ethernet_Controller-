/*
 * monitor.h
 *
 *  Created on: Jan 23, 2024
 *      Author: wypishinski-prechter
 */

#ifndef MONITOR_H_
#define MONITOR_H_

typedef struct{
	char ascii_buff[255];
	int valid;
	int size;
	int pre;
	int dest;
	int sor;
	int len;
	int crc;
	int trail;
	int init;
} buffer;

void init_leds();
void init_timers();
void init_receivepin();
int get_state();
void set_state(int new_state);
void clear_buffer();
buffer get_buffer();


#endif /* MONITOR_H_ */
