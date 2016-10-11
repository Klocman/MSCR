/*
*  Copyright 2014 Marcin Szeniak
*/

#ifndef ROLLINGAVERAGE_H_
#define ROLLINGAVERAGE_H_

#include <avr/io.h>

class RollingAverage {
	private:
	uint16_t store[4];
	uint16_t sum;
	uint8_t index;
	
	public:
	void Init (uint16_t val);
	
	void AddValue (uint16_t input);
	
	uint16_t GetValue ();
};

#endif /* ROLLINGAVERAGE_H_ */