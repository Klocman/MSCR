/*
*  Copyright 2014 Marcin Szeniak
*/

#include "Potentiometer.h"

 Potentiometer::Potentiometer()
{
	inputValue.Init(0);
}

void Potentiometer::AddRawValue(uint16_t input)
{
	inputValue.AddValue(input);
}

uint8_t Potentiometer::GetValue(void)
{
	return Clamp10To8(inputValue.GetValue());
}
