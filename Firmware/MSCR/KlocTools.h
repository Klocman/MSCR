/*
*  Copyright 2014 Marcin Szeniak
*/

#pragma once

#include <avr/io.h>

#define BIT_LOW(port,pin) port &= ~(1<<pin)
#define BIT_HIGH(port,pin) port |= (1<<pin)
#define BIT_TOGGLE(port,pin) port ^= (1<<pin)
#define BIT_READ(portpin,pin) (portpin & (1<<pin))
#define SET_PIN_INPUT(ddr,pin) ddr &= ~(1<<pin)
#define SET_PIN_OUTPUT(ddr,pin) ddr |= (1<<pin)

typedef void (*Action) (void); 

void Clamp( uint8_t* input, const uint8_t min, const uint8_t max );

void Wrap( int8_t* input, const int8_t min, const int8_t max );

void WrapSub1( uint8_t *input, const uint8_t min, const uint8_t max );

void WrapAdd1( uint8_t *input, const uint8_t min, const uint8_t max );

void ClampSub1( uint8_t *input, const uint8_t min );

void ClampAdd1( uint8_t *input, const uint8_t max );

uint16_t ClampSubtract( const uint16_t a, const uint16_t b );

uint8_t ClampDownTo8( const float in );

uint8_t Clamp10To8( const uint16_t in );

inline uint8_t Difference8 ( const uint8_t *a, const uint8_t *b )
{
	if (*a > *b)
	{
		return *a - *b;
	}
	else
	{
		return *b - *a;
	}
}

inline uint16_t Difference16 ( const uint16_t *a, const uint16_t *b )
{
	if (*a > *b)
	{
		return *a - *b;
	}
	else
	{
		return *b - *a;
	}
}

inline uint8_t ConvertToPercentage (const uint8_t *val, const uint8_t *max)
{
	return ((uint16_t)(*val) * (uint16_t)100) / *max;
}

inline void Rescale8 (uint8_t *input, const uint8_t max)
{
	*input = ((uint16_t)(*input) * (uint16_t)max) >> 8;
}