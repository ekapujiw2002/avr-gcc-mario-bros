/*
 * prjMario.c
 *
 * Created: 10/29/2012 12:53:51 PM
 * Author: EX4
 * Description :
 * LCD Connection :
PIN LCD			PIN MIKRO
 RS				PC0
 RW				PC1
 EN				PC2
 D4				PC4
 D5				PC5
 D6				PC6
 D7				PC7

* Koneksi switch :
 SWITCH			PIN MIKRO
 SW_UP			PD2
 SW_DOWN		PD3
 SW_SCROLL		PD4
 
* Koneksi led : PORTB 
 */ 

//lib
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd_lib.h"
#include "adc.h"
//#include "usart_lib.h"
#include "timer_util.h"

//define
#define DDR_LED DDRB
#define PORT_LED PORTB

#define MARIO 'M'
#define BLOCK '#'
#define SPACE ' '
#define MAX_SCORE 25

//global var
uint8_t volatile btnMode = 0;	//default up, must be volatile honey ^_^
uint8_t volatile playerState = 1; //use in ISR
uint8_t scrollMode = 0; //0 = rigth to left movement; 1 = left to right
uint8_t lcd_buff1[17], lcd_buff2[17];	//array buffer screen, will be treated as string null
uint8_t score=0; //total score

//config isr int0 dan 1
void SetupInterupsiButton()
{
	//setup port and pin button
	DDRD &= ~((1<<DDD2) | (1<<DDD3) | (1<<DD4));
	PORTD |= (1<<PD2) | (1<<PD3) | (1<<PD4);
	
	MCUCR |= (1<<ISC11) | (1<<ISC01);//falling edge int
	GIFR |= (1<<INTF1) | (1<<INTF0); //clear all pending isr
	GICR |= (1<<INT1) | (1<<INT0);	//enable int0 1
}

//loading animation
void DisplayLoadingAnimation()
{
	uint8_t c1=0;
	
	//init port led
	DDR_LED = 255;
	PORT_LED = 0;
	
	//setup timer1
	Timer1InitAsInterruptableTimer(5000);	//5s
	
	//poll the flag tov1
	PORT_LED = 3;
	while(!isTimer1Overflow())
	{
		LCDGotoXY(0,1);
		LCDprogressBar(c1, 255, 16);
		c1++;
		if (c1%5==0)
		{
			PORT_LED <<= 1;
			if (PORT_LED==0)
			{
				PORT_LED = 3;
			}
		}
		_delay_ms(19);
	}
	Timer1Stop();
	PORT_LED = 0;
}

//lcd welcome
void DisplayLCDWelcome()
{
	LCDinit();
	LCDclr();
	LCDcursorOFF();
	LCDhome();
	LCDGotoXY(3,0);
	fprintf(&LCDInputOutputStream,"AVR MARIO");
}

//display lcd buffer to screen
void DisplayLCDBuffer(uint8_t* Buffer1,uint8_t* Buffer2,uint8_t ClearScreen)
{
	if (ClearScreen)
	{
		LCDclr();
	}
	LCDGotoXY(0,0);
	fprintf(&LCDInputOutputStream,"%s",Buffer1);
	LCDGotoXY(0,1);
	fprintf(&LCDInputOutputStream,"%s",Buffer2);
}

//update mario position
void UpdateMarioPosition(uint8_t aColPosIndex, uint8_t aMarioRowIndex)
{
	LCDGotoXY(aColPosIndex,0);
	LCDstring((aMarioRowIndex) ? " ":"M",1);
	LCDGotoXY(aColPosIndex,1);
	LCDstring((aMarioRowIndex) ? "M":" ",1);
}

//cek mode sebuah blok
uint8_t GetColumnIndexBlockMode(uint8_t* Buff1, uint8_t* Buff2,uint8_t aColumnIndex)
{
	if ((Buff1[aColumnIndex]==SPACE) && (Buff2[aColumnIndex]==SPACE) )
	{
		return 0;
	}
	
	if ((Buff1[aColumnIndex]==BLOCK) && (Buff2[aColumnIndex]==SPACE) )
	{
		return 1;
	}
	
	if ((Buff1[aColumnIndex]==SPACE) && (Buff2[aColumnIndex]==BLOCK) )
	{
		return 2;
	}
	
	return 3;
}

//set mode kolom
void SetColumnIndexMode(uint8_t* Buff1, uint8_t* Buff2,uint8_t aColumnIndex,uint8_t aColumnMode)
{
	switch(aColumnMode)
	{
		case 0:
			Buff1[aColumnIndex] = Buff2[aColumnIndex] = SPACE;
			break;
			
		case 1:
			Buff1[aColumnIndex] = BLOCK;
			Buff2[aColumnIndex] = SPACE;
			break;
			
		case 2:
			Buff1[aColumnIndex] = SPACE;
			Buff2[aColumnIndex] = BLOCK;
			break;
			
		default:
			Buff1[aColumnIndex] = Buff2[aColumnIndex] = SPACE;
			break;
	}
}

//generate a block at a column position
//return block mode created
uint8_t GenerateBlockAtColumnIndex(uint8_t* Buff1, uint8_t* Buff2,uint8_t aColumnIndex,uint8_t aBlockMode)
{
	uint8_t forbiddenColumnIndex, oldBlockMode;
	
	//set forbidden mode
	switch(aBlockMode)
	{
		case 0:
			forbiddenColumnIndex = 3;
			break;
			
		case 1: 
			forbiddenColumnIndex = 2;
			break;
			
		case 2:
			forbiddenColumnIndex = 1;
			break;
		
		default:
			forbiddenColumnIndex = 3;
			break;
	}
	
	//get old blockmode
	oldBlockMode = GetColumnIndexBlockMode(Buff1,Buff2,aColumnIndex);
	
	if (aColumnIndex==0)	//index 0 => cek neighbour kanan
	{
		if (GetColumnIndexBlockMode(Buff1,Buff2,aColumnIndex+1)!=forbiddenColumnIndex)
		{
			SetColumnIndexMode(Buff1,Buff2,aColumnIndex,aBlockMode);
			return aBlockMode;
		}
	} 
	else if(aColumnIndex==15) //index 15 => cek neighbour kiri
	{
		if (GetColumnIndexBlockMode(Buff1,Buff2,aColumnIndex-1)!=forbiddenColumnIndex)
		{
			SetColumnIndexMode(Buff1,Buff2,aColumnIndex,aBlockMode);
			return aBlockMode;
		}
	}
	else //1-14
	{
		//cek neighbour kanan dan kiri
		if ((GetColumnIndexBlockMode(Buff1,Buff2,aColumnIndex+1)!=forbiddenColumnIndex) && (GetColumnIndexBlockMode(Buff1,Buff2,aColumnIndex-1)!=forbiddenColumnIndex))
		{
			SetColumnIndexMode(Buff1,Buff2,aColumnIndex,aBlockMode);
			return aBlockMode;
		} 
	}	
	
	return oldBlockMode;
}

//fill with spaces
void FillArrayWithData(uint8_t* aData, uint8_t aLen, uint8_t aDataDefault)
{
	for (uint8_t i=0;i<aLen;i++)
	{
		aData[i] = aDataDefault;
	}		
}

//count block
uint8_t CountBlockNumber(uint8_t* Buff1, uint8_t* Buff2)
{
	uint8_t cnt=0;
	for (uint8_t i=0;i<16;i++)
	{
		if (GetColumnIndexBlockMode(Buff1,Buff2,i)!=0)
		{
			cnt++;
		}
	}
	
	return cnt;
}

//updet scroll mode
void CekUpdateScrollMode(uint8_t* aScrollMode,uint8_t* Buffer1,uint8_t* Buffer2)
{
	uint8_t oldMode;
	
	if (bit_is_clear(PIND, PIND4))
	{
		loop_until_bit_is_set(PIND, PIND4);
		*aScrollMode ^= 1;
		
		//update array value
		oldMode = GetColumnIndexBlockMode(Buffer1,Buffer2, ((*aScrollMode) ? 0:15 ) );
		SetColumnIndexMode(Buffer1,Buffer2, ((*aScrollMode) ? 0:15 ), oldMode);
	}
}

//init 1st block
void InitFirstGameBlock()
{
	uint8_t c1,c2;
	
	//reset score
	score = 0;
	playerState = 1;
	btnMode = 0;
	
	//generate 5 random block init
	while( (CountBlockNumber(lcd_buff1,lcd_buff2)!=5) || (GetColumnIndexBlockMode(lcd_buff1,lcd_buff2,((scrollMode) ? 15:0))==1) )
	{
		c1 = random() % 3;	//pattern mode 0-2
		c2 = random() % 16; //col mode 0-15
		GenerateBlockAtColumnIndex(lcd_buff1,lcd_buff2,c2,c1);
	}
	lcd_buff1[((scrollMode) ? 15:0)] = MARIO;
	
	//display lcd buffer
	DisplayLCDBuffer(lcd_buff1,lcd_buff2,1);
}

//reinit 1st block
void ReinitFirstGameBlock()
{
	loop_until_bit_is_clear(PIND, PIND4);
	loop_until_bit_is_set(PIND, PIND4);
	InitFirstGameBlock();
}

//main program
int main(void)
{	
	//usart init for debugging
	//USART_Init(BAUD_19200, COM_CH0, PARITY_NONE, STOP_BIT_1, DATA_BIT_8);
	
	//init random seed, use from adc value--well random enough,hehehehe :)
	ADC_Init(ADC_VREF_AVCC, ADC_DATA_10BIT, ADC_PSC128);	//init adc
	srandom(ADC_ReadData(ADC_CH0));	//init using adc0 value
	
	//fill buffer with spaces
	FillArrayWithData(lcd_buff1, 16, SPACE);
	FillArrayWithData(lcd_buff2, 16, SPACE);
	lcd_buff1[16] = lcd_buff2[16] = 0;	//string nulled for fprintf
	
	//init lcd
	DisplayLCDWelcome();	
	DisplayLoadingAnimation();	
	
	//init block game pertama
	InitFirstGameBlock();	
	
	//setup isr tombol up down
	SetupInterupsiButton();
	sei();
	
	//main program
    while(1)
    {			
		//delay timer
		Timer1InitAsInterruptableTimer(1000/((score/5)+1));	//more speed as more score
		while(!isTimer1Overflow())
		{
			CekUpdateScrollMode(&scrollMode,lcd_buff1,lcd_buff2);
		}
		Timer1Stop();
		
		//advance block enemy to the left or right :)		
		//scroll by scrollmode and insert new random block
		for (uint8_t i= ((scrollMode) ? 15:0) ; ((scrollMode) ? i>=1:i<=14) ; ((scrollMode) ? i--:i++))
		{
			lcd_buff1[i] = lcd_buff1[((scrollMode) ? i-1:i+1)];
			lcd_buff2[i] = lcd_buff2[((scrollMode) ? i-1:i+1)];
		}
		GenerateBlockAtColumnIndex(lcd_buff1,lcd_buff2, ((scrollMode) ? 0:15) ,(random() % 3));
		
		//mario pos, check colition, updet score
		if (!btnMode)	//up mode
		{
			if (lcd_buff1[ ((scrollMode) ? 15:0) ]==SPACE)	//scroe up
			{
				score++;
			}
			else //loss
			{
				playerState = 0;				
			}
			lcd_buff1[ ((scrollMode) ? 15:0) ] = MARIO;
		}
		else //down
		{
			if (lcd_buff2[ ((scrollMode) ? 15:0) ]==SPACE)	//scroe up
			{
				score++;
			}
			else //loss
			{
				playerState = 0;
			}
			lcd_buff2[ ((scrollMode) ? 15:0) ] = MARIO;
		}
		
		//display lcd buffer
		DisplayLCDBuffer(lcd_buff1,lcd_buff2,0);
		
		//win or loss
		if (playerState)	//playing
		{
			if (score==MAX_SCORE)	//win
			{
				playerState = 0;
				LCDclr();
				LCDGotoXY(0,0);
				fprintf(&LCDInputOutputStream,"YOU WIN ^_^");
				LCDGotoXY(0,1);
				fprintf(&LCDInputOutputStream,"PRESS SCROLL");
				
				ReinitFirstGameBlock();
			}
		} 
		else //loose
		{
			LCDclr();
			LCDGotoXY(0,0);
			fprintf(&LCDInputOutputStream,"YOU LOOSE T_T %d", score);
			LCDGotoXY(0,1);
			fprintf(&LCDInputOutputStream,"PRESS SCROLL");
			
			ReinitFirstGameBlock();
		}
    }
	
	return 0; //will never go here ^_^
}

//isr int0
SIGNAL(INT0_vect)
{
	if (playerState)
	{
		btnMode = 0;
		UpdateMarioPosition( ((scrollMode) ? 15:0) , btnMode);
	}
}

//isr int1
SIGNAL(INT1_vect)
{
	if (playerState)
	{
		btnMode = 1;
		UpdateMarioPosition( ((scrollMode) ? 15:0) , btnMode);
	}
}