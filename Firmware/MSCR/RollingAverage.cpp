/*
*  Copyright 2014 Marcin Szeniak
*/

#include "RollingAverage.h"

uint16_t RollingAverage::GetValue()
{
	return sum >> 2;
}

void RollingAverage::AddValue( uint16_t input )
{
	sum -= store[index];
		store[index] = input;
		sum += store[index];
		index++;
		if (index == 4) index = 0;
}

void RollingAverage::Init( uint16_t val )
{
	for(int i=0;i<4;i++)
		{
			store[i] = val;
		}
		sum = val * 4;
		index = 0;
}

