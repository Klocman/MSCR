/*
* Copyright 2014 Marcin Szeniak
*/

#ifndef _24C64_H_
#define _24C64_H_

#define FALSE 0
#define TRUE 1
//#include <avr/io.h>
//#define F_CPU 8000000

union EEAddr16_u
{
	EEAddr16_u ()
	{
		var = 0;
	}
	uint16_t var;
	uint8_t splitVar[2];
};

void EESendStop();

uint8_t EEGoToAddr(EEAddr16_u *address);

uint8_t EEReadRelative (uint8_t*output);
uint8_t EERead4BytesRelative(uint8_t output[4]);

uint8_t EEReadByte(EEAddr16_u *address, uint8_t*output);
uint8_t EERead4Bytes(EEAddr16_u *address, uint8_t output[4]);

uint8_t EEWriteByte(EEAddr16_u *address,uint8_t *data);
uint8_t EEWrite4Bytes(EEAddr16_u *address, uint8_t data[4]);
#endif
