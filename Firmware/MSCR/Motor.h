/*
* Copyright 2014 Marcin Szeniak
*/

#pragma once

struct MotorConfiguration
{
	void Init (uint8_t min, uint8_t max)
	{
		minPwmAmount = min;
		maxPwmAmount = max;
	}
	
	uint8_t minPwmAmount;
	uint8_t maxPwmAmount;
};

class Motor
{
	private:
	volatile uint8_t *port;
	uint8_t pin1;
	uint8_t pin2;
	volatile uint8_t *pwmRegister;
	
	ControllerDirection currentDirection;
	
	public:
	MotorConfiguration configuration;
	static uint8_t Smoothing;
	
	void InitPorts(volatile uint8_t *directionPort, volatile uint8_t *directionDdr,
	const uint8_t directionPin1, const uint8_t directionPin2, volatile uint8_t *pwm)
	{
		port = directionPort;
		pin1 = directionPin1;
		pin2 = directionPin2;
		pwmRegister = pwm;
		
		SET_PIN_OUTPUT(*directionDdr, pin1);
		SET_PIN_OUTPUT(*directionDdr, pin2);
		SetDirection(None);
	}
	
	void SetDirection (ControllerDirection direction)
	{
		if (direction != currentDirection)
		{
			switch(direction)
			{
				case Negative:
				BIT_LOW(*port, pin1);
				BIT_HIGH(*port, pin2);
				break;
			
				case Positive:
				BIT_HIGH(*port, pin1);
				BIT_LOW(*port, pin2);
				break;
			
				default:
				BIT_LOW(*port, pin1);
				BIT_LOW(*port, pin2);
				break;
			}
			currentDirection = direction;
		}
	}
	
	void SetSpeed (ControllerAxisData input, uint8_t enableSmoothing)
	{
		//TODO: smoothing with proper directions
		//SetDirection(input.direction);
		
		uint8_t result = (((uint16_t)(configuration.maxPwmAmount - configuration.minPwmAmount) * (uint16_t)input.magnitude) / 0xff) + configuration.minPwmAmount;
		
		if(enableSmoothing)
			result = ((uint16_t)result * (uint16_t)(0xff-Smoothing) + (((uint16_t)*pwmRegister) * (uint16_t)(Smoothing))) >> 8;
		
		if (input.direction != None && input.direction != currentDirection)
		{
			// Prevent direction changing while PWM duty cycle is still high
			result = configuration.minPwmAmount;
		}
		
		*pwmRegister = result;//input.magnitude;
		SetDirection(input.direction);
	}
	
	void Stop ()
	{
		*pwmRegister = 0;
		SetDirection(None);
	}
};

uint8_t Motor::Smoothing = 0;

#define MotorSmoothingMaxVal 244
/*
MotorConfiguration Motor::configuration = {
	26, //10% min
	230, //90% max
	0,
	};*/

