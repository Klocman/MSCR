/*
*  Copyright 2014 Marcin Szeniak
*/

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "KlocTools.h"
#include "PinConfiguration.h"
#include <avr/wdt.h>
/*
#define BUTTONS_DDR DDRB
#define BUTTONS_PORT PORTB
#define BUTTONS_PIN PINB
#define BUTTONS_L_PIN PINB7
#define BUTTONS_R_PIN PINB6
#define BUTTONS_O_PIN PINB5
#define BUTTONS_B_PIN PINB4
*/

#define BUTTONS_CANCEL_PIN BUTTONS_R1_PIN
#define BUTTONS_OK_PIN BUTTONS_R2_PIN
#define BUTTONS_UP_PIN BUTTONS_R4_PIN
#define BUTTONS_DOWN_PIN BUTTONS_R3_PIN
#define BUTTONS_A1_PIN BUTTONS_R3_PIN
#define BUTTONS_A2_PIN BUTTONS_R2_PIN
#define BUTTONS_A3_PIN BUTTONS_R4_PIN
#define BUTTONS_JOY_PIN BUTTONS_R1_PIN

enum ButtonEvents
{
	//BUTTON_SINGLE = 0,
	BUTTON_DOWN = 0,
	BUTTON_UP = 1,
	BUTTON_OK = 2,
	BUTTON_BACK = 3,
	
	BUTTON_AUX1 = 4,
	BUTTON_AUX2 = 5,
	BUTTON_AUX3 = 6,
	BUTTON_JOY = 7,
	
	BUTTON_NONE = 0xfe,
	EVENT_MULTIPLE_BUTTONS = 0xff
};

class Buttons
{
	private:
	ButtonEvents previouslyPressed;
	uint8_t holdTimer;
	
	Action keypressAction;
	
	ButtonEvents GrabInput( void )
	{
		wdt_reset();
		
		ButtonEvents currentlyPressed = BUTTON_NONE;
		
		// Make sure only one column is driven low
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_C2_PIN);
		// Drive column 1 low to detect button presses
		SET_PIN_OUTPUT(BUTTONS_DDR, BUTTONS_C1_PIN);
		// Allow some time for pins to stabilize
		_delay_us(10);
		
		// If bit is low the button is pressed (row pull-up gets driven down by the column)
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_CANCEL_PIN))
		{
			currentlyPressed = BUTTON_BACK;
		}
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_OK_PIN))
		{
			if (currentlyPressed != BUTTON_NONE) return EVENT_MULTIPLE_BUTTONS;
			currentlyPressed = BUTTON_OK;
		}
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_UP_PIN))
		{
			if (currentlyPressed != BUTTON_NONE) return EVENT_MULTIPLE_BUTTONS;
			currentlyPressed = BUTTON_UP;
		}
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_DOWN_PIN))
		{
			if (currentlyPressed != BUTTON_NONE) return EVENT_MULTIPLE_BUTTONS;
			currentlyPressed = BUTTON_DOWN;
		}
		
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_C1_PIN);
		SET_PIN_OUTPUT(BUTTONS_DDR, BUTTONS_C2_PIN);
		_delay_us(10);
		
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_A1_PIN))
		{
			if (currentlyPressed != BUTTON_NONE) return EVENT_MULTIPLE_BUTTONS;
			currentlyPressed = BUTTON_AUX1;
		}
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_A2_PIN))
		{
			if (currentlyPressed != BUTTON_NONE) return EVENT_MULTIPLE_BUTTONS;
			currentlyPressed = BUTTON_AUX2;
		}
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_A3_PIN))
		{
			if (currentlyPressed != BUTTON_NONE) return EVENT_MULTIPLE_BUTTONS;
			currentlyPressed = BUTTON_AUX3;
		}
		if (!BIT_READ(BUTTONS_PIN, BUTTONS_JOY_PIN))
		{
			if (currentlyPressed != BUTTON_NONE) return EVENT_MULTIPLE_BUTTONS;
			currentlyPressed = BUTTON_JOY;
		}
		
		return currentlyPressed;
	}
	
	public:
	
	void InitPorts (Action keypressCallback)
	{
		// Set up button rows to be inputs with pull-ups
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_R1_PIN);
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_R2_PIN);
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_R3_PIN);
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_R4_PIN);
		BIT_HIGH(BUTTONS_PORT, BUTTONS_R1_PIN);
		BIT_HIGH(BUTTONS_PORT, BUTTONS_R2_PIN);
		BIT_HIGH(BUTTONS_PORT, BUTTONS_R3_PIN);
		BIT_HIGH(BUTTONS_PORT, BUTTONS_R4_PIN);
		
		// Set columns to be high-z for now. Later columns will be individually set to output, resulting in driving them low
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_C1_PIN);
		SET_PIN_INPUT(BUTTONS_DDR, BUTTONS_C2_PIN);
		BIT_LOW(BUTTONS_PORT, BUTTONS_C1_PIN);
		BIT_LOW(BUTTONS_PORT, BUTTONS_C2_PIN);
		
		previouslyPressed = BUTTON_NONE;
		holdTimer = 0;
		
		keypressAction = keypressCallback;
	}
	
	// Return currently pressed button
	ButtonEvents GetKeyEvent (void)
	{
		_delay_ms(60);
		
		ButtonEvents currentlyPressed = GrabInput();
		previouslyPressed = currentlyPressed;
		if (currentlyPressed != BUTTON_NONE)
		{
			keypressAction();
		}
		return currentlyPressed;
	}
	
	// -Debounce, -keep pressed for a bit to enable rapid fire
	// -filter out multiple buttons pressed at one time
	ButtonEvents WaitForKeyEvent (void)
	{
		ButtonEvents currentlyPressed;
		while(1)
		{
			_delay_ms(20);
			currentlyPressed = GrabInput();
			_delay_ms(20);
			if (currentlyPressed != GrabInput())
			{
				continue;
			}
			_delay_ms(20);
			if (currentlyPressed != GrabInput())
			{
				continue;
			}
			
			switch(currentlyPressed)
			{
				case BUTTON_DOWN:
				case BUTTON_UP:
				case BUTTON_BACK:
				case BUTTON_OK:
				{
					if (previouslyPressed == BUTTON_NONE)
					{
						previouslyPressed = currentlyPressed;
						keypressAction();
						return currentlyPressed;
					}
					else if (previouslyPressed == EVENT_MULTIPLE_BUTTONS)
					{
						continue;
					} 
					else if (previouslyPressed == currentlyPressed)
					{
						if (holdTimer >= 0x06)
						{
							keypressAction();
							return currentlyPressed;
						}
						else
						{
							holdTimer++;
						}
					}
				}
				break;
				
				case BUTTON_NONE:
				case EVENT_MULTIPLE_BUTTONS:
				{
					holdTimer = 0;
				}
				break;
				
				default:
				//TODO: Aux switches
				break;
			}
			previouslyPressed = currentlyPressed;
		}
	}
};

#endif