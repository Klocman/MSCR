/*
* Copyright 2014 Marcin Szeniak
*/

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "LCDTestCPP/aux_globals.h"
#include "ControllerAxis.h"
#include "Buttons.h"
#include "KlocTools.h"
#include "Motor.h"
#include "PinConfiguration.h"
#include "Potentiometer.h"
#include "EEPROM/ExtEemem.h"
#include "EEPROM/24c64.h"

#define VERSION_INFO "Firmware:   v1.1\n08/09/2014"

#define VOLTAGE_MAX 170
#define VOLTAGE_MIN 52

struct VoltMeterSettings
{
	float voltageConversionRatio = 0.0f;
	uint8_t alarmValue = VOLTAGE_MIN;
};

class VoltMeter
{
	public:
	RollingAverage inputValue;
	VoltMeterSettings settings;

	void Init()
	{
		inputValue.Init(0);
	}
	void AddRawValue (uint16_t input) // Add raw value from ADC to be processed.
	{
		inputValue.AddValue(input);
	}
	uint8_t GetVoltage (void)
	{
		float x = (float)inputValue.GetValue() * settings.voltageConversionRatio;
		return (uint8_t)(x*10);
	}
};

enum AdcControl
{
	ADC_NORMAL = 0,
	ADC_BUSY = 1,
	ADC_CAL_XYZ = 2,
	ADC_CAL_MID = 3
};

enum ControllerState
{
	ControllerNormal,
	ControllerRecordingPlaying,
	ControllerHoldValue,
};

enum DisplayedValue
{
	DisplayMenu = 0,
	DisplayP1 = 1,
	DisplayP2 = 2,
	DisplayP3 = 3,
	DisplayB1 = 5,
	DisplayB2 = 6,
	DisplayB3 = 7,
};

enum RecordingSettings
{
	Recording30hz = 0, // 1:1 recording
	Recording15hz = 1, // 2:1
	Recording10hz = 2, // 3:1
	//Recording8hz = 3, // 3.75:1
	//Recording6hz = 4, // 5:1
};

enum DisplayOKThingyType
{
	OKThingyTypeRightOK = 0,
	OKThingyTypeLeftOK,
};

#define AXIS_CONTROL_SWAPXY 0
#define AXIS_CONTROL_DISABLEX 1
#define AXIS_CONTROL_DISABLEY 2
#define AXIS_CONTROL_DISABLEZ 3

#define DISPLAY_TIMEOUT_BOOT 40

#define BACKLIGHT_TIMEOUT_MAX 245
#define BACKLIGHT_TIMEOUT_MIN 76

// 8 seconds, less than 255
#define POTENTIOMETER_TIMEOUT_MAX 245
#define POTENTIOMETER_TIMEOUT_MIN 15

struct GeneralSettings
{
	RecordingSettings recordingFrequency = Recording30hz;
	uint8_t axisControlRegister = 0;
	uint8_t backlightTimeout = BACKLIGHT_TIMEOUT_MAX;
	uint8_t potentiometerTimeout = 45;
	uint8_t kickstartMagnitude = 75;
};

#define DisplayEepromStartSavingReading() currentCtrlState = ControllerRecordingPlaying; currentlyDisplayedValue = DisplayMenu
#define DisplayEepromFinishSavingReading() currentCtrlState = ControllerNormal; currentlyDisplayedValue = DisplayMenu

#define BACKLIGHT_ON() BIT_LOW(DISPLAY_BL_PORT, DISPLAY_BL_PIN)
#define BACKLIGHT_OFF() BIT_HIGH(DISPLAY_BL_PORT, DISPLAY_BL_PIN)

#define MControlNormal 0
#define MControlStillLoading 1
#define MControlQueuedFinishRecordPlay 2
#define MControlUnused 4



#define  KICKSTART_CYCLES 2

volatile uint8_t miscControlRegister = (1<<MControlStillLoading);

GeneralSettings settings;

ControllerAxis xAxis;
ControllerAxis yAxis;
ControllerAxis zAxis;

ControllerAxisData xLastValue;
ControllerAxisData yLastValue;
ControllerAxisData zLastValue;

Motor xMotor;
Motor yMotor;
Motor zMotor;

Potentiometer pot1;
Potentiometer pot2;
Potentiometer pot3;

uint8_t pot2LastValue = 0; //Buffered to avoid converting back from float

HD44780 lcd;
Buttons buttons;
ExtEememInterface extMem;

VoltMeter supplyVoltage;
volatile uint8_t lastSupplyVoltage;

volatile AdcControl adcControl = ADC_NORMAL;
uint16_t calStorage[6];

volatile ControllerState currentCtrlState = ControllerNormal;
volatile DisplayedValue currentlyDisplayedValue = DisplayMenu;

uint8_t displayTimeout = DISPLAY_TIMEOUT_BOOT;
uint8_t backlightTimeout = 0xff;

volatile uint8_t recordingDivideCounter = 0;


#pragma region EEPROM

#define EEvalidValue 0x12
uint8_t EEMEM EEvalid;
ControllerAxisConfiguration EEMEM EExAxisConfig;
ControllerAxisConfiguration EEMEM EEyAxisConfig;
ControllerAxisConfiguration EEMEM EEzAxisConfig;
MotorConfiguration EEMEM EExMotorConfig;
MotorConfiguration EEMEM EEyMotorConfig;
MotorConfiguration EEMEM EEzMotorConfig;
uint8_t EEMEM EEMotorSmoothing;
GeneralSettings EEMEM EEgeneralSettings;
VoltMeterSettings EEMEM EEvoltMeterSettings;

void SaveSettings ()
{
	eeprom_write_block(&xAxis.configuration, &EExAxisConfig, sizeof(ControllerAxisConfiguration));
	eeprom_write_block(&yAxis.configuration, &EEyAxisConfig, sizeof(ControllerAxisConfiguration));
	eeprom_write_block(&zAxis.configuration, &EEzAxisConfig, sizeof(ControllerAxisConfiguration));

	eeprom_write_block(&xMotor.configuration, &EExMotorConfig, sizeof(MotorConfiguration));
	eeprom_write_block(&yMotor.configuration, &EEyMotorConfig, sizeof(MotorConfiguration));
	eeprom_write_block(&zMotor.configuration, &EEzMotorConfig, sizeof(MotorConfiguration));
	eeprom_write_byte(&EEMotorSmoothing, Motor::Smoothing);
	
	eeprom_write_block(&settings, &EEgeneralSettings, sizeof(GeneralSettings));
	eeprom_write_block(&supplyVoltage.settings, &EEvoltMeterSettings, sizeof(VoltMeterSettings));

	eeprom_write_byte(&EEvalid, EEvalidValue);
}

void ReadSettings ()
{
	if(eeprom_read_byte(&EEvalid) != EEvalidValue)
	return;

	eeprom_read_block(&xAxis.configuration, &EExAxisConfig, sizeof(ControllerAxisConfiguration));
	eeprom_read_block(&yAxis.configuration, &EEyAxisConfig, sizeof(ControllerAxisConfiguration));
	eeprom_read_block(&zAxis.configuration, &EEzAxisConfig, sizeof(ControllerAxisConfiguration));
	
	eeprom_read_block(&xMotor.configuration, &EExMotorConfig, sizeof(MotorConfiguration));
	eeprom_read_block(&yMotor.configuration, &EEyMotorConfig, sizeof(MotorConfiguration));
	eeprom_read_block(&zMotor.configuration, &EEzMotorConfig, sizeof(MotorConfiguration));
	Motor::Smoothing = eeprom_read_byte(&EEMotorSmoothing);
	
	eeprom_read_block(&settings, &EEgeneralSettings, sizeof(GeneralSettings));
	eeprom_read_block(&supplyVoltage.settings, &EEvoltMeterSettings, sizeof(VoltMeterSettings));
}

#pragma endregion EEPROM

#pragma region MenuMethods

uint8_t Message_P ( const char *message );
void ScrollMenu_P ( const char strings[], const Action actions[], const uint8_t count );
void ChangeOption_P ( const char *text, uint8_t *variable, uint8_t min, uint8_t max );

void ClearCalStorage ()
{
	for (uint8_t i = 0; i < 3; i++)
	{
		calStorage[i] = 0xffff;
	}
}

void CalibrateXYZCalculateDeadMid(ControllerAxisConfiguration *conf, uint16_t *low, uint16_t *high)
{
	uint16_t deadRange = ClampSubtract(*high, *low) / 2;
	if (deadRange > 0xff)
	{
		deadRange = 0xff;
	}
	conf->middlePoint = *low + deadRange;
	conf->deadRange = deadRange + 4;
}

void CalibrateXYZ()
{
	CalibrateXYZstep0:
	adcControl = ADC_BUSY;
	if (!Message_P(PSTR("Sprawdz limity\njoysticka")))
	{
		adcControl = ADC_NORMAL;
		return;
	}
	ClearCalStorage();
	adcControl = ADC_CAL_XYZ;

	ButtonEvents tempButton;

	while (1)
	{
		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("X%.3u  Y%.3u  Z%.3u\n%.4u  %.4u  %.4u"),
		calStorage[0], calStorage[1], calStorage[2], calStorage[3], calStorage[4], calStorage[5]);

		tempButton = buttons.GetKeyEvent();
		if (tempButton == BUTTON_OK)
		{
			xAxis.configuration.bottomPoint = calStorage[0];
			xAxis.configuration.topPoint = calStorage[3];
			yAxis.configuration.bottomPoint = calStorage[1];
			yAxis.configuration.topPoint = calStorage[4];
			zAxis.configuration.bottomPoint = calStorage[2];
			zAxis.configuration.topPoint = calStorage[5];
			break;
		}
		else if (tempButton == BUTTON_BACK)
		{
			goto CalibrateXYZstep0;
		}
	}

	CalibrateXYZstep1:
	adcControl = ADC_BUSY;

	if (!Message_P(PSTR("Ustaw joystick\nw pozycji 0")) || !Message_P(PSTR("Sprawdz luzy\njoysticka")))
	{
		goto CalibrateXYZstep0;
	}

	ClearCalStorage();
	adcControl = ADC_CAL_MID;

	while (1)
	{
		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("X%.3u  Y%.3u  Z%.3u\n %.3u   %.3u   %.3u"),
		calStorage[0], calStorage[1], calStorage[2], calStorage[3], calStorage[4], calStorage[5]);

		tempButton = buttons.GetKeyEvent();
		if (tempButton == BUTTON_OK)
		{
			CalibrateXYZCalculateDeadMid(&xAxis.configuration, &calStorage [0], &calStorage[3]);
			CalibrateXYZCalculateDeadMid(&yAxis.configuration, &calStorage [1], &calStorage[4]);
			CalibrateXYZCalculateDeadMid(&zAxis.configuration, &calStorage [2], &calStorage[5]);
			break;
		}
		else if (tempButton == BUTTON_BACK)
		{
			goto CalibrateXYZstep1;
		}
	}

	adcControl = ADC_BUSY;
	if (!Message_P(PSTR("Kalibracja\nzakonczona")))
	{
		goto CalibrateXYZstep1;
	}
	adcControl = ADC_NORMAL;
}

void ViewAnalogData()
{
	while (buttons.GetKeyEvent() != BUTTON_BACK)
	{
		ControllerAxisData x = xAxis.GetValue();
		ControllerAxisData y = yAxis.GetValue();
		ControllerAxisData z = zAxis.GetValue();

		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("Pozycje: X%c%.3u\nY%c%.3u Z%c%.3u "), x.GetDirectionSign(), x.magnitude
		, y.GetDirectionSign(), y.magnitude, z.GetDirectionSign(), z.magnitude);
	}
}

void ViewXYZCalData()
{
	uint8_t scrollPosition = 0;
	while(1)
	{
		ControllerAxisConfiguration config;
		char axis;
		switch(scrollPosition)
		{
			case 0:
			axis = 'X';
			config = xAxis.configuration;
			break;
			case 1:
			axis = 'Y';
			config = yAxis.configuration;
			break;
			default:
			axis = 'Z';
			config = zAxis.configuration;
			break;
		}

		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("%c Limit %.3u/%.4u\nMid %.3u Dead %.3u"), axis, config.bottomPoint,
		config.topPoint, config.middlePoint, config.deadRange);

		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_BACK:
			return;

			case BUTTON_UP:
			ClampSub1(&scrollPosition, 0);
			break;

			case BUTTON_DOWN:
			ClampAdd1(&scrollPosition, 2);
			break;

			default:
			break;
		}
	}
}

void DisplayOKThingy (const DisplayOKThingyType type)
{
	lcd.lcd_gotoxy(12,1);
	lcd.lcd_string_P(type == OKThingyTypeRightOK ? PSTR("R\x7EOK") : PSTR("L\x7EOK"));
}

uint8_t Message_P ( const char *message )
{
	lcd.lcd_clrscr();
	lcd.lcd_home();
	lcd.lcd_string_format_P(message);
	DisplayOKThingy(OKThingyTypeRightOK);

	while (1)
	{
		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_OK:
			return 1;

			case BUTTON_BACK:
			return 0;

			default:
			break;
		}
	}
}

void ScrollMenu_P( const char *strings[], const Action actions[], const uint8_t count )
{
	uint8_t scrollPosition = 0;
	while(1)
	{
		lcd.lcd_clrscr();
		lcd.lcd_home();
		lcd.lcd_char('\x7E');
		lcd.lcd_string_P(strings[scrollPosition]);
		lcd.lcd_gotoxy(0,1);
		lcd.lcd_char('\xA5');
		lcd.lcd_string_P(strings[(scrollPosition+1) % count]);
		//lcd.lcd_string_format_P(PSTR("\x7E%s\n\xA5%s"), strings[scrollPosition], strings[(scrollPosition+1) % count]);

		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_UP:
			WrapSub1(&scrollPosition, 0, count-1);
			break;
			case BUTTON_DOWN:
			WrapAdd1(&scrollPosition, 0, count-1);
			break;

			case BUTTON_OK:
			actions[scrollPosition]();
			break;

			case BUTTON_BACK:
			return;

			default:
			break;
		}
	}
}

void ViewBatteryInfo ()
{
	if (supplyVoltage.settings.voltageConversionRatio == 0)
	{
		Message_P(PSTR("Brak kalibracji\nbaterii"));
		return;
	}
	lcd.lcd_clrscr();
	lcd.lcd_home();
	lcd.lcd_string_P(PSTR("Stan baterii:"));
	DisplayOKThingy(OKThingyTypeLeftOK);
	do
	{
		lcd.lcd_gotoxy(0,1);
		lcd.lcd_string_format_P(PSTR("%.2u,%.1uV"), lastSupplyVoltage/10, lastSupplyVoltage%10);
	} while (buttons.GetKeyEvent() != BUTTON_BACK);
}

inline void ChangeOptionHeader_P (const char *text)
{
	lcd.lcd_clrscr();
	lcd.lcd_home();
	lcd.lcd_string_P(text);
	DisplayOKThingy(OKThingyTypeRightOK);
}

void ChangeVoltageOption_P(const char *text, uint8_t *variable)
{
	ChangeOptionHeader_P(text);
	uint8_t tempVariable = *variable;
	while(1)
	{
		lcd.lcd_gotoxy(0,1);
		lcd.lcd_string_format_P(PSTR("%2u,%1uV"), tempVariable/10, tempVariable%10);

		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_UP:
			ClampAdd1(&tempVariable, VOLTAGE_MAX);
			break;

			case BUTTON_DOWN:
			ClampSub1(&tempVariable, VOLTAGE_MIN);
			break;

			case BUTTON_OK:
			*variable = tempVariable;

			case BUTTON_BACK:
			return;

			default:
			break;
		}
	}
}

void CalibrateBattery()
{
	uint8_t tempVar = supplyVoltage.GetVoltage();

	BatteryCalStep1:

	if (!Message_P(PSTR("Podaj napiecie\nzasilania")))
	{
		return;
	}

	ChangeVoltageOption_P(PSTR("Obecne napiecie"), &tempVar);

	if (!Message_P(PSTR("Kalibracja\nzakonczona.")))
	{
		goto BatteryCalStep1;
	}

	float tempVarf = ((float)tempVar)/10.0f;
	supplyVoltage.settings.voltageConversionRatio = tempVarf / (float)(supplyVoltage.inputValue.GetValue()-1);
}

void ChangeRecordingResolution()
{
	ChangeOptionHeader_P(PSTR("Rozdzielczosc"));
	RecordingSettings tempValue = settings.recordingFrequency;
	while(1)
	{
		lcd.lcd_gotoxy(0,1);
		uint8_t result;
		switch(tempValue)
		{
			default:
			settings.recordingFrequency = Recording30hz;
			tempValue = Recording30hz;
			case Recording30hz:
			result = 30;
			break;
			case Recording15hz:
			result = 15;
			break;
			case Recording10hz:
			result = 10;
			break;
		}

		lcd.lcd_string_format_P(PSTR("%2upkt / 1s"), result);

		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_UP:
			ClampSub1((uint8_t*)(&tempValue), (uint8_t)Recording30hz);
			break;

			case BUTTON_DOWN:
			ClampAdd1((uint8_t*)(&tempValue), (uint8_t)Recording10hz);
			break;

			case BUTTON_OK:
			settings.recordingFrequency = (RecordingSettings)tempValue;
			case BUTTON_BACK:
			return;

			default:
			break;
		}
	}
}

void ChangeOption_P( const char *text, uint8_t *variable, const uint8_t min, const uint8_t max )
{
	ChangeOptionHeader_P(text);
	uint8_t tempVariable = *variable;
	while(1)
	{
		lcd.lcd_gotoxy(0,1);
		lcd.lcd_string_format_P(PSTR("%3u %3u%%"), tempVariable, ConvertToPercentage(&tempVariable, &max));

		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_UP:
			ClampAdd1(&tempVariable, max);
			break;

			case BUTTON_DOWN:
			ClampSub1(&tempVariable, min);
			break;

			case BUTTON_OK:
			*variable = tempVariable;

			case BUTTON_BACK:
			return;

			default:
			break;
		}
	}
}

void ChangeBit_P( const char *text, uint8_t *targetRegister, const uint8_t targetBit)
{
	ChangeOptionHeader_P(text);
	uint8_t tempVariable = BIT_READ(*targetRegister,targetBit);
	while(1)
	{
		lcd.lcd_gotoxy(0,1);
		lcd.lcd_string_P(tempVariable == 0 ? PSTR("Nie") : PSTR("Tak"));

		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_UP:
			tempVariable = 1;
			break;

			case BUTTON_DOWN:
			tempVariable = 0;
			break;

			case BUTTON_OK:
			if (tempVariable)
			{
				BIT_HIGH(*targetRegister,targetBit);
			}
			else
			{
				BIT_LOW(*targetRegister,targetBit);
			}

			case BUTTON_BACK:
			return;

			default:
			break;
		}
	}
}

void ChangeTimeOption_P( const char *text, uint8_t *variable, const uint8_t min, const uint8_t max, const uint8_t showMinMax)
{
	ChangeOptionHeader_P(text);
	uint8_t tempVariable = *variable;
	while(1)
	{
		lcd.lcd_gotoxy(0,1);
		if(showMinMax && tempVariable <= min)
		{
			lcd.lcd_string_P(PSTR("Wylaczone"));
		}
		else if (showMinMax && (tempVariable >= max))
		{
			lcd.lcd_string_P(PSTR("Wlaczone"));
		}
		else
		{
			uint16_t result = ((uint16_t)tempVariable*10)/30;
			lcd.lcd_string_format_P(PSTR("%u,%us     "), result/10, result%10);
		}

		switch(buttons.WaitForKeyEvent())
		{
			case BUTTON_UP:
			if(tempVariable < max)
			{
				tempVariable += 3;
			}
			if (tempVariable > max)
			{
				tempVariable = max;
			}
			break;

			case BUTTON_DOWN:
			if(tempVariable > min)
			{
				tempVariable -= 3;
			}
			if (tempVariable < min)
			{
				tempVariable = min;
			}
			break;

			case BUTTON_OK:
			*variable = tempVariable;
			return;

			case BUTTON_BACK:
			return;

			default:
			break;
		}
	}
}

void DebugExtEeprom ()
{
	lcd.lcd_clrscr();
	lcd.lcd_gotoxy(12,1);
	DisplayOKThingy(OKThingyTypeLeftOK);

	EEAddr16_u addr;
	uint8_t editing = 0;
	while (1)
	{
		uint8_t result = 0;
		EEReadByte(&addr, &result);
		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("Addr: %.4u%c\nVal: %.3u%c"), addr.var, editing ? ' ' : '<', result, editing ? '<' : ' ');

		switch(buttons.WaitForKeyEvent())
		{
			case  BUTTON_BACK:
			if (editing)
			{
				editing = 0;
			}
			else
			{
				return;
			}
			break;

			case BUTTON_UP:
			if (editing)
			{
				result++;
				EEWriteByte(&addr, &result);
			}
			else
			{
				addr.var++;
				if(addr.var > EEtop)
				addr.var = 0;
			}
			break;

			case BUTTON_DOWN:
			if (editing)
			{
				result--;
				EEWriteByte(&addr, &result);
			}
			else
			{
				addr.var--;
				if(addr.var > EEtop)
				addr.var = EEtop;
			}
			break;

			case BUTTON_OK:
			editing = 1;
			break;

			default:
			break;
		}
	}
}

void DebugPots ()
{
	lcd.lcd_clrscr();
	do
	{
		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("P1: %.3u, P2: %.3u\nP3: %.3u"), pot1.GetValue(), pot2.GetValue(), pot3.GetValue());

		_delay_ms(20);
	}
	while (buttons.GetKeyEvent() != BUTTON_BACK);
}

void DebugButtons ()
{
	lcd.lcd_clrscr();
	ButtonEvents result = BUTTON_NONE;
	do
	{
		result = buttons.GetKeyEvent();

		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("1:%.1u 2:%.1u 3:%.1u E:%.1u\nU:%.1u D:%.1u L:%.1u R:%.1u"),
		result == BUTTON_AUX1, result == BUTTON_AUX2, result == BUTTON_AUX3, result == BUTTON_JOY,
		result == BUTTON_UP, result == BUTTON_DOWN, result == BUTTON_BACK, result == BUTTON_OK);
	}
	while (result != EVENT_MULTIPLE_BUTTONS);
}

void DebugPWM ()
{
	lcd.lcd_clrscr();
	DisplayOKThingy(OKThingyTypeLeftOK);
	do
	{
		lcd.lcd_home();
		lcd.lcd_string_format_P(PSTR("M1: %.3u, M2: %.3u\nM3: %.3u"), M1_E_OCR, M2_E_OCR, M3_E_OCR);
	}
	while (buttons.GetKeyEvent() != BUTTON_BACK);
}

void DisplayVersionInfo ()
{
	while (Message_P(PSTR(VERSION_INFO)))
	{
		while (Message_P(PSTR("Autor: Marcin\nSzeniak")))
		{
			return;
		}
	}
}

void GuiSaveSettings ()
{
	SaveSettings();
	Message_P(PSTR("Ustawienia zost.\nzapisane."));
}

#pragma endregion MenuMethods

#pragma region MenuLists
void SetupMenuLists ()
{

}
//////////////////////////////////////////////////////////////////////////
// Menus // Strings can have 15 chars and 1 null at the end
#define motorMenuCount 7
const char PROGMEM motorMenuStrings_0[16] PROGMEM = "Moc kopniecia";
const char PROGMEM motorMenuStrings_1[16] PROGMEM = "X Maks moc";
const char PROGMEM motorMenuStrings_2[16] PROGMEM = "X Min moc";

const char PROGMEM motorMenuStrings_3[16] PROGMEM = "Y Maks moc";
const char PROGMEM motorMenuStrings_4[16] PROGMEM = "Y Min moc";

const char PROGMEM motorMenuStrings_5[16] PROGMEM = "Z Maks moc";
const char PROGMEM motorMenuStrings_6[16] PROGMEM = "Z Min moc";
const char *motorMenuStrings[motorMenuCount] = {
	///////////////
	motorMenuStrings_0,
	motorMenuStrings_1,
	motorMenuStrings_2,
	motorMenuStrings_3,
	motorMenuStrings_4,
	motorMenuStrings_5,
	motorMenuStrings_6
};
const Action motorMenuActions[motorMenuCount] = {
	[] () {ChangeOption_P(motorMenuStrings[0], &settings.kickstartMagnitude, 0, 0xff);},
	[] () {ChangeOption_P(motorMenuStrings[1], &xMotor.configuration.maxPwmAmount, 0, 0xff);},
	[] () {ChangeOption_P(motorMenuStrings[2], &xMotor.configuration.minPwmAmount, 0, 0xff);},
	[] () {ChangeOption_P(motorMenuStrings[3], &yMotor.configuration.maxPwmAmount, 0, 0xff);},
	[] () {ChangeOption_P(motorMenuStrings[4], &yMotor.configuration.minPwmAmount, 0, 0xff);},
	[] () {ChangeOption_P(motorMenuStrings[5], &zMotor.configuration.maxPwmAmount, 0, 0xff);},
	[] () {ChangeOption_P(motorMenuStrings[6], &zMotor.configuration.minPwmAmount, 0, 0xff);},
};

#define MENU_MOTOR motorMenuStrings, motorMenuActions, motorMenuCount

//////////////////////////////////////////////////////////////////////////
#define joystickMenuCount 7
const char PROGMEM joystickMenuStrings_0[16] PROGMEM = "Pozycja galki";
const char PROGMEM joystickMenuStrings_1[16] PROGMEM = "Inwersja X";
const char PROGMEM joystickMenuStrings_2[16] PROGMEM = "Inwersja Y";
const char PROGMEM joystickMenuStrings_3[16] PROGMEM = "Inwersja Z";
const char PROGMEM joystickMenuStrings_4[16] PROGMEM = "Zamien X z Y";
const char PROGMEM joystickMenuStrings_5[16] PROGMEM = "Kalibruj...";
const char PROGMEM joystickMenuStrings_6[16] PROGMEM = "Obecna kalibr.";
const char *joystickMenuStrings[joystickMenuCount] = {
	///////////////
	joystickMenuStrings_0,
	joystickMenuStrings_1,
	joystickMenuStrings_2,
	joystickMenuStrings_3,
	joystickMenuStrings_4,
	joystickMenuStrings_5,
	joystickMenuStrings_6,
};
const Action joystickMenuActions[joystickMenuCount] = {
	ViewAnalogData,
	[] () {ChangeBit_P(joystickMenuStrings[1], &xAxis.configuration.invert, 0);},
	[] () {ChangeBit_P(joystickMenuStrings[2], &yAxis.configuration.invert, 0);},
	[] () {ChangeBit_P(joystickMenuStrings[3], &zAxis.configuration.invert, 0);},
	[] () {ChangeBit_P(joystickMenuStrings[4], &settings.axisControlRegister, AXIS_CONTROL_SWAPXY);},
	CalibrateXYZ,
	ViewXYZCalData,
};

#define MENU_JOY joystickMenuStrings, joystickMenuActions, joystickMenuCount

//////////////////////////////////////////////////////////////////////////
#define batteryMenuCount 2
const char PROGMEM batteryMenuStrings_0[16] PROGMEM = "Poziom alarmowy";
const char PROGMEM batteryMenuStrings_1[16] PROGMEM = "Kalibruj...";
const char *batteryMenuStrings[batteryMenuCount] = {
	///////////////
	batteryMenuStrings_0,
	batteryMenuStrings_1,
};
const Action batteryMenuActions[batteryMenuCount] = {
	[] () {ChangeVoltageOption_P(batteryMenuStrings[0], &supplyVoltage.settings.alarmValue);},
	CalibrateBattery,
};

#define MENU_BATT batteryMenuStrings, batteryMenuActions, batteryMenuCount

//////////////////////////////////////////////////////////////////////////
#define debugMenuCount 6
const char PROGMEM debugMenuStrings_0[16] PROGMEM = "Wersja firmw.";
const char PROGMEM debugMenuStrings_1[16] PROGMEM = "Przyciski";
const char PROGMEM debugMenuStrings_2[16] PROGMEM = "Pokretla";
const char PROGMEM debugMenuStrings_3[16] PROGMEM = "EEPROM reader";
const char PROGMEM debugMenuStrings_4[16] PROGMEM = "EEPROM errors";
const char PROGMEM debugMenuStrings_5[16] PROGMEM = "PWM outputs";
const char *debugMenuStrings[debugMenuCount] = {
	///////////////
	debugMenuStrings_0,
	debugMenuStrings_1,
	debugMenuStrings_2,
	debugMenuStrings_3,
	debugMenuStrings_4,
	debugMenuStrings_5,
};
const Action debugMenuActions[debugMenuCount] = {
	DisplayVersionInfo,
	DebugButtons,
	DebugPots,
	DebugExtEeprom,
	[] () {ChangeOption_P(debugMenuStrings[4], &extMem.operationErrors, 0, 255);},
	DebugPWM,
};

#define MENU_DEBUG debugMenuStrings, debugMenuActions, debugMenuCount

//////////////////////////////////////////////////////////////////////////
#define settingsMenuCount 7
//const char PROGMEM settingsMenuStrings_0[16] PROGMEM = "Zapisz ustaw.";
const char PROGMEM settingsMenuStrings_0[16] PROGMEM = "Joystick      >";
const char PROGMEM settingsMenuStrings_1[16] PROGMEM = "Silniki       >";
const char PROGMEM settingsMenuStrings_2[16] PROGMEM = "Bateria       >";
const char PROGMEM settingsMenuStrings_3[16] PROGMEM = "Debug/Testy   >";
const char PROGMEM settingsMenuStrings_4[16] PROGMEM = "Czas podgladu";
const char PROGMEM settingsMenuStrings_5[16] PROGMEM = "Podswietlanie";
const char PROGMEM settingsMenuStrings_6[16] PROGMEM = "FPS nagrywania";
const char *settingsMenuStrings[settingsMenuCount] = {
	///////////////
	settingsMenuStrings_0,
	settingsMenuStrings_1,
	settingsMenuStrings_2,
	settingsMenuStrings_3,
	settingsMenuStrings_4,
	settingsMenuStrings_5,
	settingsMenuStrings_6
};
const Action settingsMenuActions[settingsMenuCount] = {
	//GuiSaveSettings,
	[] () {ScrollMenu_P(MENU_JOY);},
	[] () {ScrollMenu_P(MENU_MOTOR);},
	[] () {ScrollMenu_P(MENU_BATT);},
	[] () {ScrollMenu_P(MENU_DEBUG);},
	[] () {ChangeTimeOption_P(settingsMenuStrings[5], &settings.potentiometerTimeout, POTENTIOMETER_TIMEOUT_MIN, POTENTIOMETER_TIMEOUT_MAX, 0);},
	[] () {ChangeTimeOption_P(settingsMenuStrings[6], &settings.backlightTimeout, BACKLIGHT_TIMEOUT_MIN, BACKLIGHT_TIMEOUT_MAX, 1);},
	ChangeRecordingResolution,
};

#define MENU_SETTINGS settingsMenuStrings, settingsMenuActions, settingsMenuCount

//////////////////////////////////////////////////////////////////////////
#define onoffAxisMenuCount 3
const char PROGMEM onoffAxisMenuStrings_0[16] PROGMEM = "Wylacz X";
const char PROGMEM onoffAxisMenuStrings_1[16] PROGMEM = "Wylacz Y";
const char PROGMEM onoffAxisMenuStrings_2[16] PROGMEM = "Wylacz Z";
const char *onoffAxisMenuStrings[onoffAxisMenuCount] = {
	///////////////
	onoffAxisMenuStrings_0,
	onoffAxisMenuStrings_1,
	onoffAxisMenuStrings_2,
};
const Action onoffAxisMenuActions[onoffAxisMenuCount] = {
	[] () {ChangeBit_P(onoffAxisMenuStrings[0], &settings.axisControlRegister, AXIS_CONTROL_DISABLEX);},
	[] () {ChangeBit_P(onoffAxisMenuStrings[1], &settings.axisControlRegister, AXIS_CONTROL_DISABLEY);},
	[] () {ChangeBit_P(onoffAxisMenuStrings[2], &settings.axisControlRegister, AXIS_CONTROL_DISABLEZ);},
};

#define MENU_ONOFFAXIS onoffAxisMenuStrings, onoffAxisMenuActions, onoffAxisMenuCount

//////////////////////////////////////////////////////////////////////////
#define mainMenuCount 3
const char PROGMEM mainMenuStrings_0[16] PROGMEM = "Wylacz osie   >";
const char PROGMEM mainMenuStrings_1[16] PROGMEM = "Stan baterii";
const char PROGMEM mainMenuStrings_2[16] PROGMEM = "Ustawienia    >";
const char *mainMenuStrings[mainMenuCount] = {
	///////////////
	mainMenuStrings_0,
	mainMenuStrings_1,
	mainMenuStrings_2,
};
const Action mainMenuActions[mainMenuCount] = {
	[] () {ScrollMenu_P(MENU_ONOFFAXIS);},
	ViewBatteryInfo,
	[] () {ScrollMenu_P(MENU_SETTINGS);},
};

#define MENU_MAIN mainMenuStrings, mainMenuActions, mainMenuCount

#pragma endregion MenuLists

void MemBankDisplay (const ButtonEvents *currentButtonEvent, const EememBank bank)
{
	lcd.lcd_string_format_P(PSTR("   Nagranie %.1u   \nU\x7EPlay  D\x7ERecord"), (uint8_t)bank + 1);

	switch(*currentButtonEvent)
	{
		case BUTTON_UP:
		extMem.Start(bank, EEmReading);
		DisplayEepromStartSavingReading();
		break;

		case BUTTON_DOWN:
		extMem.Start(bank, EEmSaving);
		currentCtrlState = ControllerNormal;
		currentlyDisplayedValue = DisplayMenu;
		DisplayEepromStartSavingReading();
		break;

		case BUTTON_BACK:
		case BUTTON_OK:
		currentlyDisplayedValue = DisplayMenu;
		DisplayEepromFinishSavingReading();
		break;

		default:
		break;
	}
}

inline void PrintAxisInfo (ControllerAxisData *data, const uint8_t disabled)
{
	if (disabled)
	{
		lcd.lcd_string_P(PSTR(" OFF"));
	}
	else
	{
		lcd.lcd_string_format_P(PSTR("%c%.3u"), data->GetDirectionSign(), data->magnitude);
	}
}

void StartMainMenu( void )
{
	ButtonEvents currentButtonEvent = BUTTON_NONE;
	ButtonEvents previousButtonEvent = BUTTON_NONE;

	uint8_t warningFlashTimer = 0;
	uint8_t showWarning = 0;

	// Don't allow to break out
	while(1)
	{
		//////////////////////////////////////////////////////////////////////////
		// Input processing
		currentButtonEvent = buttons.GetKeyEvent();

		// Require a cycle with no buttons pressed to accept next input
		if (previousButtonEvent != BUTTON_NONE)
		{
			previousButtonEvent = currentButtonEvent;
			currentButtonEvent = BUTTON_NONE;
		}
		else
		{
			previousButtonEvent = currentButtonEvent;
		}

		// "Global" hotkeys
		switch(currentButtonEvent)
		{
			case BUTTON_AUX1:
			case BUTTON_AUX2:
			case BUTTON_AUX3:
			//if (currentlyDisplayedValue < DisplayB1)
			{
				BIT_HIGH(miscControlRegister,MControlQueuedFinishRecordPlay);
				currentCtrlState = ControllerNormal;
				currentlyDisplayedValue = (DisplayedValue)(currentButtonEvent - BUTTON_AUX1 + DisplayB1);
			}
			break;

			case BUTTON_JOY:
			BIT_HIGH(miscControlRegister,MControlQueuedFinishRecordPlay);
			currentlyDisplayedValue = DisplayMenu;

			switch(currentCtrlState)
			{
				case ControllerRecordingPlaying:
				case ControllerHoldValue:
				currentCtrlState = ControllerNormal;
				break;

				case ControllerNormal:
				currentCtrlState = ControllerHoldValue;
				break;
			}
			break;

			default:
			break;
		}

		//////////////////////////////////////////////////////////////////////////
		// Display stuff
		lcd.lcd_home();
		switch(currentlyDisplayedValue)
		{
			case DisplayMenu:
			//Draw first line depending on what is going on with the controls
			switch(currentCtrlState)
			{
				case ControllerNormal:
				{
					lcd.lcd_string_P(PSTR("X|Y|Z "));

					if (warningFlashTimer > 0)
					{
						warningFlashTimer--;
					}
					else
					{
						if (lastSupplyVoltage <= supplyVoltage.settings.alarmValue)
						{
							showWarning = !showWarning;
							warningFlashTimer = 10;
						}
						else
						{
							showWarning = 0;
						}
					}

					lcd.lcd_string_P(showWarning ? PSTR("BAT") : PSTR("   "));
					lcd.lcd_string_P(PSTR(" R\x7EMenu"));

					// Open main menu
					if (currentButtonEvent == BUTTON_OK)
					{
						ScrollMenu_P(MENU_MAIN);
						SaveSettings();
					}
				}
				break;

				case ControllerRecordingPlaying:
				{
					switch(extMem.GetCurrentMode())
					{
						case EEmReading:
						lcd.lcd_string_format_P(PSTR("Odczyt\x7E%.1u %.2u/100%%"), extMem.GetCurrentBank()+1, extMem.GetCurrentPercentage());
						break;

						case EEmSaving:
						lcd.lcd_string_format_P(PSTR("Zapisz\x7E%.1u %.2u/100%%"), extMem.GetCurrentBank()+1, extMem.GetCurrentPercentage());
						break;

						default:
						currentCtrlState = ControllerNormal;
						break;
					}

					switch(currentButtonEvent)
					{
						case BUTTON_UP:
						case BUTTON_DOWN:
						case BUTTON_OK:
						case BUTTON_BACK:
						BIT_HIGH(miscControlRegister,MControlQueuedFinishRecordPlay);
						DisplayEepromFinishSavingReading();
						break;

						default:
						break;
					}
				}
				break;

				case ControllerHoldValue:
				lcd.lcd_string_P(PSTR(" Blokada  galki "));
				break;
			}

			//Draw second line (resulting controller data)
			lcd.lcd_gotoxy(0,1);
			lcd.lcd_char(' ');
			PrintAxisInfo(&xLastValue, BIT_READ(settings.axisControlRegister, AXIS_CONTROL_DISABLEX));
			lcd.lcd_char(' ');
			PrintAxisInfo(&yLastValue, BIT_READ(settings.axisControlRegister, AXIS_CONTROL_DISABLEY));
			lcd.lcd_char(' ');
			PrintAxisInfo(&zLastValue, BIT_READ(settings.axisControlRegister, AXIS_CONTROL_DISABLEZ));
			lcd.lcd_char(' ');

			/*
			lcd.lcd_string_format_P(PSTR(" %c%.3u %c%.3u %c%.3u "),
			xLastValue.GetDirectionSign(), xLastValue.magnitude,
			yLastValue.GetDirectionSign(), yLastValue.magnitude,
			zLastValue.GetDirectionSign(), zLastValue.magnitude);
			*/
			break;

			case DisplayP1:
			{
				uint8_t tempTop = MotorSmoothingMaxVal-1;
				lcd.lcd_string_format_P(PSTR("Wygladzanie     \n    %.3u/100%%    "), ConvertToPercentage(&Motor::Smoothing, &tempTop));
			}
			break;

			case DisplayP2:
			lcd.lcd_string_format_P(PSTR("Limit predkosci \n    %.3u/255      "), pot2LastValue);
			break;

			case DisplayP3:
			{
				uint8_t tempTop = 0xff;
				lcd.lcd_string_format_P(PSTR("Przyspieszenie  \n    %.3u/100%%    "), ConvertToPercentage(&ControllerAxis::globalConfiguration.maxSpeedChange, &tempTop));
			}
			break;

			case DisplayB1:
			MemBankDisplay(&currentButtonEvent, EE0);
			break;

			case DisplayB2:
			MemBankDisplay(&currentButtonEvent, EE1);
			break;

			case DisplayB3:
			MemBankDisplay(&currentButtonEvent, EE2);
			break;
		}
	}
}


int main(void)
{
	//////////////////////////////////////////////////////////////////////////
	// Setup LCD first and display splash
	lcd.lcd_init();
	lcd.lcd_string_P(PSTR("Uruchamianie ...\nMSCR1  M.Szeniak"));
	lcd.lcd_cursor_mode(HD44780::Invisible);
	lcd.lcd_gotoxy(14,0);

	// Turn on LCD back light
	SET_PIN_OUTPUT(DISPLAY_BL_DDR, DISPLAY_BL_PIN);
	BACKLIGHT_ON();

	//////////////////////////////////////////////////////////////////////////
	//Begin startup configuration
	BIT_LOW(ASSR, AS2); // Disable timer2 clock source from external oscillator (need pins for I/O)

	// PWM on timer 2
	SET_PIN_OUTPUT(M3_E_DDR, M3_E_PIN); // OC2 is now an output
	OCR2 = 0; // set PWM for 0% duty cycle (off)
	TCCR2 |= (1 << WGM20) | (1 << WGM21); // set phase correct PWM Mode
	TCCR2 |= (1 << COM21); //| (1 << COM20); // set non-inverting mode
	TCCR2 |= (1 << CS22) ;//| (1 << CS20); // set prescaler to 32 and start PWM (around 500Hz with phase correct PWM)

	// PWM on timer 1
	// OC1A and OC1B are now outputs
	SET_PIN_OUTPUT(M1_E_DDR, M1_E_PIN);
	SET_PIN_OUTPUT(M2_E_DDR, M2_E_PIN);
	// set PWM for 0% duty cycle (off)
	OCR1A = 0;
	OCR1B = 0;
	// set phase correct PWM Mode (8bit)
	TCCR1A |= (1 << WGM10) | (1 << WGM11);
	// set non-inverting mode
	TCCR1A |= (1 << COM1A1);
	TCCR1A |= (1 << COM1B1);
	// set prescaler to 32 and start PWM (around 500Hz with phase correct PWM)
	TCCR1B |= (1 << CS12);// | (1 << CS10);

	xMotor.InitPorts(&M3_12_PORT, &M3_12_DDR, M3_1_PIN, M3_2_PIN, &M3_E_OCR);
	yMotor.InitPorts(&M2_12_PORT, &M2_12_DDR, M2_1_PIN, M2_2_PIN, &M2_E_OCR);
	zMotor.InitPorts(&M1_12_PORT, &M1_12_DDR, M1_1_PIN, M1_2_PIN, &M1_E_OCR);


	xMotor.configuration.maxPwmAmount = 0xff;
	yMotor.configuration.maxPwmAmount = 0xff;
	zMotor.configuration.maxPwmAmount = 0xff;
	xMotor.configuration.minPwmAmount = 26;
	yMotor.configuration.minPwmAmount = 42;
	zMotor.configuration.minPwmAmount = 38;
	
	
	// Set motors to 0 speed until done loading
	ControllerAxisData temp;
	xMotor.SetSpeed(temp, 0);
	yMotor.SetSpeed(temp, 0);
	zMotor.SetSpeed(temp, 0);

	// Interrupts on timer 0
	BIT_HIGH(TCCR0,CS02);
	BIT_HIGH(TCCR0,CS00);
	BIT_HIGH(TIMSK,TOIE0);

	// Disable OC0 pin
	BIT_LOW(TCCR0,COM00);
	BIT_LOW(TCCR0,COM01);

	// Disable analog comparator
	BIT_LOW(ACSR, ACIE);
	BIT_LOW(ACSR, ACI);
	BIT_LOW(ACSR, ACIC);
	BIT_HIGH(ACSR, ACD);

	// ADC setup
	ADMUX = 0;       // Reset ADC settings and use ADC0
	ADMUX |= (1 << REFS0);    // use AVcc as the reference
	//ADMUX |= (1 << ADLAR);  // Right adjust for 8 bit resolution
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // 128 prescaler for 8Mhz system clock
	//ADCSRA |= (1 << ADATE);    // Set auto trigger
	//SFIOR &= !((1 << ADTS0)|(1 << ADTS1)|(1 << ADTS2)); // Set auto trigger to free running mode
	ADCSRA |= (1 << ADEN);    // Enable the ADC
	ADCSRA |= (1 << ADIE);    // Enable ADC conversion complete interrupt

	// TWI setup for 200kHz
	TWBR = 3;
	BIT_HIGH(TWSR, TWPS0);
	BIT_LOW(TWSR, TWPS1);
	// Enable pull-ups for TWI
	SET_PIN_INPUT(PORTC, PINC0);
	SET_PIN_INPUT(PORTC, PINC1);
	BIT_HIGH(PORTC, PINC0);
	BIT_HIGH(PORTC, PINC1);

	buttons.InitPorts([] () {
		backlightTimeout = settings.backlightTimeout;
	});
	ReadSettings();

	sei();
	ADCSRA |= (1 << ADSC);    // Start the ADC conversion

	while (miscControlRegister);

	// Watchdog will reset the AVR if program will not read button states for an extended period of time
	wdt_enable(WDTO_1S);

	StartMainMenu();
}

#pragma region EventTimer


uint8_t xKickStartCounter = 0;
uint8_t yKickStartCounter = 0;
uint8_t zKickStartCounter = 0;
//Running at 30.6Hz
ISR(TIMER0_OVF_vect)
{
	uint8_t stillLoading = BIT_READ(miscControlRegister, MControlStillLoading);

	ControllerDirection xdir = xLastValue.direction;
	ControllerDirection ydir = yLastValue.direction;
	ControllerDirection zdir = zLastValue.direction;
	
	
	if(xdir == None)
	{
		xKickStartCounter = 0;
	}
	if(ydir == None)
	{
		yKickStartCounter = 0;
	}
	if(zdir == None)
	{
		zKickStartCounter = 0;
	}

	// Sanity checks
	if (currentCtrlState == ControllerRecordingPlaying && extMem.GetCurrentMode() == EEmStopped)
	{
		currentCtrlState = ControllerNormal;
		extMem.Finish();
	}

	// Acquire new values
	switch(currentCtrlState)
	{
		case ControllerRecordingPlaying:
		if (extMem.GetCurrentMode() == EEmReading)
		{
			// If reading from EEPROM skip processing joystick inputs
			break;
		}
		case ControllerNormal:
		xLastValue = xAxis.GetValue();
		yLastValue = yAxis.GetValue();
		zLastValue = zAxis.GetValue();
		break;

		case ControllerHoldValue:
		// Do nothing, so axis values doesn't changed, but motor speed still gets refreshed
		break;
	}

	// Handle recording/playing
	if (currentCtrlState == ControllerRecordingPlaying)
	{
		// Divide the resolution by set amount
		if (recordingDivideCounter >= settings.recordingFrequency)
		{
			recordingDivideCounter = 0;
			// If recording this will save values to EEPROM, if reading it will read them from EEPROM
			extMem.NextPoint(&xLastValue,&yLastValue,&zLastValue);
		}
		else
		{
			recordingDivideCounter++;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Pots
	uint8_t temp = pot1.GetValue();
	Rescale8(&temp, MotorSmoothingMaxVal);
	if (!stillLoading && Difference8(&temp, &Motor::Smoothing) >= 2)
	{
		currentlyDisplayedValue = DisplayP1;
		displayTimeout = settings.potentiometerTimeout;
	}
	Motor::Smoothing = temp;

	temp = pot2.GetValue();
	if (!stillLoading && Difference8(&temp, &pot2LastValue) >= 2)
	{
		currentlyDisplayedValue = DisplayP2;
		displayTimeout = settings.potentiometerTimeout;
	}
	pot2LastValue = temp;
	ControllerAxis::globalConfiguration.maxValue = (float)temp;

	temp = pot3.GetValue();
	if (!stillLoading && Difference8(&temp, &ControllerAxis::globalConfiguration.maxSpeedChange) >= 2)
	{
		currentlyDisplayedValue = DisplayP3;
		displayTimeout = settings.potentiometerTimeout;
	}
	ControllerAxis::globalConfiguration.maxSpeedChange = temp;

	// Potentiometer display and boot timeout
	if (displayTimeout > 0)
	{
		displayTimeout--;
		if (displayTimeout == 0)
		{
			if (currentlyDisplayedValue <= DisplayP3)
			{
				currentlyDisplayedValue = DisplayMenu;
			}
			BIT_LOW(miscControlRegister, MControlStillLoading);
		}
	}

	// Backlight timeout
	if (settings.backlightTimeout < BACKLIGHT_TIMEOUT_MAX)
	{
		if (settings.backlightTimeout <= BACKLIGHT_TIMEOUT_MIN)
		{
			BACKLIGHT_OFF();
		}
		else
		{
			if (backlightTimeout > 0)
			{
				backlightTimeout--;
				BACKLIGHT_ON();
			}
			else
			{
				BACKLIGHT_OFF();
			}
		}
	}

	// Supply voltage stuff
	lastSupplyVoltage = supplyVoltage.GetVoltage();

	// Process control register
	if (BIT_READ(miscControlRegister, MControlQueuedFinishRecordPlay))
	{
		_delay_ms(4);
		extMem.Finish();
		BIT_LOW(miscControlRegister, MControlQueuedFinishRecordPlay);
	}
	
	// Output enabled axes to motors
	if (!stillLoading)
	{
		if (BIT_READ(settings.axisControlRegister, AXIS_CONTROL_DISABLEX)) xMotor.Stop();
		else
		{
			
			if(xKickStartCounter < KICKSTART_CYCLES)
			{
				
				ControllerAxisData temp = xLastValue;
				temp.magnitude = settings.kickstartMagnitude;
				xMotor.SetSpeed(temp, 0);
				
				xKickStartCounter++;
			}
			else
				xMotor.SetSpeed(xLastValue, 1);
		}
		
		if (BIT_READ(settings.axisControlRegister, AXIS_CONTROL_DISABLEY)) yMotor.Stop();
		else
		{
			
			if(yKickStartCounter < KICKSTART_CYCLES)
			{
				ControllerAxisData temp = yLastValue;
				temp.magnitude = settings.kickstartMagnitude;
				yMotor.SetSpeed(temp, 0);
				yKickStartCounter++;
			}
			else
				yMotor.SetSpeed(yLastValue, 1);
		}
		
		if (BIT_READ(settings.axisControlRegister, AXIS_CONTROL_DISABLEZ)) zMotor.Stop();
		else
		{
			
			if(zKickStartCounter++ < KICKSTART_CYCLES)
			{
				ControllerAxisData temp = zLastValue;
				temp.magnitude = settings.kickstartMagnitude;
				zMotor.SetSpeed(temp, 0);
				zKickStartCounter++;
			}
			else
				zMotor.SetSpeed(zLastValue, 1);
		}
	}
	else
	{
		// Stuff to do if still loading
		ControllerAxis::globalConfiguration.maxSpeedChange = 0xff;
	}
}

#pragma endregion EventTimer

#pragma region AdcStuff

volatile uint16_t adcValue = 0;

static void AdcCalCapture (const uint8_t offset)
{
	if (calStorage[0 + offset] == 0xffff)
	{
		calStorage[0 + offset] = adcValue;
		calStorage[3 + offset] = adcValue;
	}
	else if (adcValue < calStorage[0 + offset])
	{
		calStorage[0 + offset] = adcValue;
	}
	else if (adcValue > calStorage[3 + offset])
	{
		calStorage[3 + offset] = adcValue;
	}
}

ISR(ADC_vect)
{
	adcValue = ADC;

	uint8_t adcMux = ADMUX & 0x07;//0b00000111;
	ADMUX = (ADMUX & 0xE0) | ((adcMux + 1) & 0x07); // 0b11100000 Go to next mux and wrap around 7 -> 0
	BIT_HIGH(ADCSRA, ADSC); // Start next conversion

	switch(adcMux)
	{
		case AXIS_X_ADC_BIT:
		case AXIS_Y_ADC_BIT:
		{
			uint8_t swapXY = BIT_READ(settings.axisControlRegister, AXIS_CONTROL_SWAPXY);
			// Swap X and Y joystick axes if enabled
			if ((adcMux == AXIS_X_ADC_BIT && !swapXY) || (adcMux == AXIS_Y_ADC_BIT && swapXY))
			{
				switch(adcControl)
				{
					case ADC_CAL_MID:
					case ADC_CAL_XYZ:
					AdcCalCapture(AXIS_X_CAL_OFFSET);
					break;
					case ADC_NORMAL:
					xAxis.AddRawValue(adcValue);
					break;

					default:
					break;
				}
			}
			else
			{
				switch(adcControl)
				{
					case ADC_CAL_MID:
					case ADC_CAL_XYZ:
					AdcCalCapture(AXIS_Y_CAL_OFFSET);
					break;

					case ADC_NORMAL:
					yAxis.AddRawValue(adcValue);
					break;

					default:
					break;
				}
			}
		}
		break;

		case AXIS_Z_ADC_BIT:
		switch(adcControl)
		{
			case ADC_CAL_MID:
			case ADC_CAL_XYZ:
			AdcCalCapture(AXIS_Z_CAL_OFFSET);
			break;

			case ADC_NORMAL:
			zAxis.AddRawValue(adcValue);
			break;

			default:
			break;
		}
		break;

		case POT_1_ADC_BIT:
		pot1.AddRawValue(adcValue);
		break;

		case POT_2_ADC_BIT:
		pot2.AddRawValue(adcValue);
		break;

		case POT_3_ADC_BIT:
		pot3.AddRawValue(adcValue);
		break;

		case VBAT_SENSE_ADC_BIT:
		supplyVoltage.AddRawValue(adcValue);
		break;

		case EXTRA_ADC_BIT:
		// Who knows
		break;
	}
}
#pragma endregion AdcStuff