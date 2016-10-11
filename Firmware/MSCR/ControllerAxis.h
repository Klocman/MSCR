/*
*  Copyright 2014 Marcin Szeniak
*/

#pragma once

#ifndef _ControllerAxis_H_
#define _ControllerAxis_H_

#include <avr/io.h>
#include "RollingAverage.h"
#include "KlocTools.h"

//uint16_t ClampSubtract (uint16_t a, uint16_t b);

//uint8_t ClampDownTo8 (float in);

struct ControllerAxisConfiguration {
	void Init (uint16_t middle, uint8_t dead, uint16_t bottom, uint16_t top)
	{
		middlePoint = middle;
		deadRange = dead;
		bottomPoint = bottomPoint;
		topPoint = top;
	}
	uint16_t middlePoint;
	uint8_t deadRange;
	uint16_t bottomPoint;
	uint16_t topPoint;
	
	uint8_t invert = 0;
	
	uint16_t middlePointLow ()
	{
		return middlePoint - deadRange;
	}
	uint16_t middlePointHigh ()
	{
		return middlePoint + deadRange;
	}
};

struct ControllerAxisGlobalConfiguration {
	float maxValue = 255;
	uint8_t maxSpeedChange = 255;
};

enum ControllerDirection {
	Negative = 0,
	Positive = 1,
	None = 2,
};
struct ControllerAxisData {
	uint8_t magnitude = 0;
	ControllerDirection direction = None;
	
	char GetDirectionSign ()
	{
		switch(direction)
		{
			case Negative:
			return '-';
			case Positive:
			return '+';
			default:
			return ' ';
		}
	}
};

class ControllerAxis {
	private:
	RollingAverage inputValue;
	uint16_t previousInputValue = 0;
	
	public:
	ControllerAxisConfiguration configuration;
	static ControllerAxisGlobalConfiguration globalConfiguration;
	
	ControllerAxis ();
	ControllerAxis( ControllerAxisConfiguration config );
	void AddRawValue (uint16_t input); // Add raw value from ADC to be processed.
	ControllerAxisData GetValue (void); // Get normalized position of the controller
};

#endif