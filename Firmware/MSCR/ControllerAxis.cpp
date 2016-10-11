/*
*  Copyright 2014 Marcin Szeniak
*/

#include "ControllerAxis.h"

ControllerAxisGlobalConfiguration ControllerAxis::globalConfiguration;

void ControllerAxis::AddRawValue( uint16_t input )
{
	inputValue.AddValue(input);
}

ControllerAxisData ControllerAxis::GetValue( void )
{
	ControllerAxisData output;
	
	uint16_t rawAvg = inputValue.GetValue();
	
	// Acceleration limiting
	if (globalConfiguration.maxSpeedChange <= (0xff-2))
	{
		uint8_t maxSpeedChange = (globalConfiguration.maxSpeedChange + 2)/5; // Add 2 because 3/255 is 1%, therefore (3+2)/5 = 1
		if (Difference16(&rawAvg, &previousInputValue) > maxSpeedChange)
		{
			if (rawAvg > previousInputValue)
			{
				rawAvg = previousInputValue + maxSpeedChange;
			}
			else
			{
				rawAvg = previousInputValue - maxSpeedChange;
			}
		}
		previousInputValue = rawAvg;
	}
	
	uint16_t val;
	float mod;
		
	//Convert 10 bit representation to 8 bit with direction
	if (rawAvg < configuration.middlePointLow())
	{
		val = configuration.middlePointLow() - ClampSubtract(rawAvg, configuration.bottomPoint);
		//mod = (float)255 / (float)(configuration.middlePointLow() - configuration.bottomPoint);
		mod = globalConfiguration.maxValue / (float)(configuration.middlePointLow() - configuration.bottomPoint);
		output.direction = Negative;
	} 
	else if (rawAvg > configuration.middlePointHigh())
	{
		val = rawAvg - configuration.middlePointHigh();
		//mod = (float)255 / (float)(configuration.topPoint - configuration.middlePointHigh());
		mod = globalConfiguration.maxValue / (float)(configuration.topPoint - configuration.middlePointHigh());
		output.direction = Positive;
	}
	else
	{
		return output; // Still has default values (0, None)
	}
		
	output.magnitude = ClampDownTo8(val * mod);
	
	// Sort out the direction value
	if (!output.magnitude)
	{
		// If magnitude ends up to be 0 set direction to none
		output.direction = None;
	}
	else
	{
		if(configuration.invert)
		{
			if(output.direction == Negative)
			{
				output.direction = Positive;
			}
			else
			{
				output.direction = Negative;
			}
		}
	}
	
	return output;
}


ControllerAxis::ControllerAxis( ControllerAxisConfiguration config )
{
	configuration = config;
	inputValue.Init(config.middlePoint);
}

ControllerAxis::ControllerAxis()
{
	configuration.Init(511,30,0,1023);
	inputValue.Init(511);
}

