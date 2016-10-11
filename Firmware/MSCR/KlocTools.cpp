/*
*  Copyright 2014 Marcin Szeniak
*/

#include "KlocTools.h"
//#include "ControllerAxis.h"

void Clamp(uint8_t* input, const uint8_t min, const uint8_t max)
{
	if(*input < min)
	{
		*input = min;
	}
	else if (*input > max)
	{
		*input = max;
	}
}

uint16_t ClampSubtract(const uint16_t a, const uint16_t b)
{
	if (a > b)
	{
		return a-b;
	}
	else
	{
		return 0;
	}
}

void ClampAdd1(uint8_t *input, const uint8_t max)
{
	if(*input < max)
	{
		(*input)++;
	}
}

void ClampSub1(uint8_t *input, const uint8_t min)
{
	if(*input > min)
	{
		(*input)--;
	}
}

void WrapAdd1(uint8_t *input, const uint8_t min, const uint8_t max)
{
	if(*input == max)
	{
		*input = min;
	}
	else
	{
		(*input)++;
	}
}

void WrapSub1(uint8_t *input, const uint8_t min, const uint8_t max)
{
	if(*input == min)
	{
		*input = max;
	}
	else
	{
		(*input)--;
	}
}

void Wrap(int8_t* input, const int8_t min, const int8_t max)
{
	if(*input < min)
	{
		*input = max;
	}
	else if (*input > max)
	{
		*input = min;
	}
}


uint8_t ClampDownTo8( const float in )
{
	if (in > 255)
	{
		return 0xff;
	}
	else if (in < 0)
	{
		return 0;
	}
	return (uint8_t)in;
}

uint8_t Clamp10To8( const uint16_t in )
{
	return in >> 2;
}
