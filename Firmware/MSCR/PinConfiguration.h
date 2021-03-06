/*
*  Copyright 2014 Marcin Szeniak
*/

#ifndef PINCONFIGURATION_H_
#define PINCONFIGURATION_H_

#define M1_1_PIN PIND6
#define M1_2_PIN PIND3
#define M1_12_PORT PORTD
#define M1_12_DDR DDRD
#define M1_E_PIN PIND5
#define M1_E_PORT PORTD
#define M1_E_DDR DDRD
#define M1_E_OCR OCR1AL

#define M2_1_PIN PINB1
#define M2_2_PIN PINB0
#define M2_12_PORT PORTB
#define M2_12_DDR DDRB
#define M2_E_PIN PIND4
#define M2_E_PORT PORTD
#define M2_E_DDR DDRD
#define M2_E_OCR OCR1BL

#define M3_1_PIN PIND1
#define M3_2_PIN PIND2
#define M3_12_PORT PORTD
#define M3_12_DDR DDRD
#define M3_E_PIN PIND7
#define M3_E_PORT PORTD
#define M3_E_DDR DDRD
#define M3_E_OCR OCR2

#define DISPLAY_BL_PIN PIND0
#define DISPLAY_BL_PORT PORTD
#define DISPLAY_BL_DDR DDRD

#define BUTTONS_R1_PIN PINB7
#define BUTTONS_R2_PIN PINB6
#define BUTTONS_R3_PIN PINB5
#define BUTTONS_R4_PIN PINB4
#define BUTTONS_C1_PIN PINB3
#define BUTTONS_C2_PIN PINB2
#define BUTTONS_PORT PORTB
#define BUTTONS_DDR DDRB
#define BUTTONS_PIN PINB

#define AXIS_X_ADC_BIT ADC2_BIT
#define AXIS_Y_ADC_BIT ADC6_BIT
#define AXIS_Z_ADC_BIT ADC3_BIT
#define POT_1_ADC_BIT ADC4_BIT
#define POT_2_ADC_BIT ADc5_BIT
#define POT_3_ADC_BIT ADC7_BIT
#define EXTRA_ADC_BIT ADC1_BIT
#define VBAT_SENSE_ADC_BIT ADC0_BIT

#define AXIS_X_CAL_OFFSET 0
#define AXIS_Y_CAL_OFFSET 1
#define AXIS_Z_CAL_OFFSET 2


#endif /* PINCONFIGURATION_H_ */