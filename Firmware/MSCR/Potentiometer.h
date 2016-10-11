/*
*  Copyright 2014 Marcin Szeniak
*/

#ifndef POTENTIOMETER_H_
#define POTENTIOMETER_H_

#include "RollingAverage.h"
#include "KlocTools.h"

class Potentiometer {
	private:
	RollingAverage inputValue;
	
	public:
	Potentiometer ();
	void AddRawValue (uint16_t input); // Add raw value from ADC to be processed.
	uint8_t GetValue (void); // Get position of the potentiometer
};

#endif /* POTENTIOMETER_H_ */