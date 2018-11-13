/*
 * adc.h
 *
 * Created: 6/21/2012 10:22:44 AM
 *  Author: EX4
 */ 


#ifndef ADC_H_
#define ADC_H_

//define
#define ADC_VREF_AREF	0
#define ADC_VREF_AVCC	1	
#define ADC_VREF_INTERNAL	3

#define ADC_DATA_8BIT	1
#define ADC_DATA_10BIT	0

#define ADC_CH0	0
#define ADC_CH1	1
#define ADC_CH2	2
#define ADC_CH3	3
#define ADC_CH4	4
#define ADC_CH5	5
#define ADC_CH6	6
#define ADC_CH7	7

#define ADC_PSC2	1
#define ADC_PSC4	(ADC_PSC2+1)
#define ADC_PSC8	(ADC_PSC2+2)
#define ADC_PSC16	(ADC_PSC2+3)
#define ADC_PSC32	(ADC_PSC2+4)
#define ADC_PSC64	(ADC_PSC2+5)
#define ADC_PSC128	(ADC_PSC2+6)

void ADC_Init(const uint8_t Vref, const uint8_t DataBit, const uint8_t Psc);
uint16_t ADC_ReadData(const uint8_t aChannel);

#endif /* ADC_H_ */