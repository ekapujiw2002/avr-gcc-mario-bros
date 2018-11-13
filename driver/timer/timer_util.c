/*
 * timer_util.c
 *
 * Created: 10/31/2012 3:26:30 AM
 *  Author: EX4
 */ 

#include "timer_util.h"

//timer setup
//psc=1024
//xtal=11059200
void Timer1InitAsInterruptableTimer(uint16_t aTimerms)
{
	TCCR1B = 0;	//make sure its dead
	TIFR |= (1<<TOV1);	//clear tov1 flag
	TCNT1 = 65536 - (((11059200*aTimerms)/1000)/1024);
	TCCR1A = 0;	//init tccr1a
	TCCR1B |= (1<<CS10) | (1<<CS12);	//psc = 1024, tmr1 start
}

//timer 1 overflow?
uint8_t isTimer1Overflow()
{
	return bit_is_set(TIFR, TOV1);
}

//stop timer 1
void Timer1Stop()
{
	TCCR1B = 0;
}
