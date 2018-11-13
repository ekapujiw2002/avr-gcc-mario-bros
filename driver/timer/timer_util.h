/*
 * timer_util.h
 *
 * Created: 10/31/2012 3:26:15 AM
 *  Author: EX4
 */ 


#ifndef TIMER_UTIL_H_
#define TIMER_UTIL_H_

#include <avr/io.h>

//proto declare
void Timer1InitAsInterruptableTimer(uint16_t aTimerms);
uint8_t isTimer1Overflow();
void Timer1Stop();




#endif /* TIMER_UTIL_H_ */