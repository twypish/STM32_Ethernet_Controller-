/*
 * transmitter.h
 *
 *  Created on: Jan 31, 2024
 *      Author: Hampel Matthew
 */

#ifndef TRANSMITTER_H_
#define TRANSMITTER_H_

void init_transmitter();
void transmit(char* message, int address, int length);
void clear_trans_message();
int get_failed_status();
void set_failed_status(int fail);

#endif
