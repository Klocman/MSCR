/*
* Copyright 2014 Marcin Szeniak
*/

#ifndef EXTEEMEM_H_
#define EXTEEMEM_H_

#include <avr/io.h>
#include "../ControllerAxis.h"
#include "24c64.h"

#define EEsize 0x0aaa //2 bytes for end addr, leaving 682 4 byte blocks
#define EE0offset 0x0000
#define EE1offset 0x0aaa
#define EE2offset 0x1554
#define EEtop 0x1fff //8191 bytes

static const uint16_t EEoffsets[4] = {EE0offset, EE1offset, EE2offset, EEtop};

enum EememBank
{
	EEnone = 3,
	EE0 = 0,
	EE1 = 1,
	EE2 = 2,
};

enum EememMode
{
	EEmStopped = 0,
	EEmReading,
	EEmSaving
};

struct EememReadResult
{
	ControllerAxisData x,y,z;
};

class ExtEememInterface
{
	private:
	void ClearBank (EememBank b);
	EEAddr16_u readTopAddr;
	EEAddr16_u currentAddr;
	
	EememBank currentBank = EEnone;
	EememMode currentMode = EEmStopped;
	
	public:
	uint8_t operationErrors = 0;
	
	inline EememBank GetCurrentBank ()
	{
		return currentBank;
	}
	inline EememMode GetCurrentMode ()
	{
		return currentMode;
	}
	
	uint8_t GetCurrentPercentage ();
	
	void Start (EememBank b, EememMode m);
	void NextPoint (ControllerAxisData *x, ControllerAxisData *y, ControllerAxisData *z);
	void Finish ();
};


#endif /* EXTEEMEM_H_ */