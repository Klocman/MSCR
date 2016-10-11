/*
* Copyright 2014 Marcin Szeniak
*/

#include "ExtEemem.h"

uint8_t clearBankValue = 0xff;
void ExtEememInterface::ClearBank(EememBank b)
{
	EEWriteByte((EEAddr16_u*)&EEoffsets[b], &clearBankValue);
}

uint8_t ExtEememInterface::GetCurrentPercentage()
{
	if (currentMode == EEmStopped || currentBank == EEnone)
	{
		return 0;
	}
	
	uint16_t topPart = 10 * (currentAddr.var - EEoffsets[currentBank]);
	uint16_t botPart = (currentMode == EEmReading) ? (readTopAddr.var - EEoffsets[currentBank])/10 : EEsize/10;
	return topPart/botPart;
}

void ExtEememInterface::Start(EememBank b, EememMode m)
{
	if (m == EEmStopped || b == EEnone)
	{
		Finish();
		return;
	}
	
	currentAddr.var = EEoffsets[b];
	uint8_t addrBuffer = 0;
	if (!EEReadByte(&currentAddr, &addrBuffer)) operationErrors++;
	
	currentBank = b;
	currentMode = m;
	currentAddr.var +=2; //Skip over the address bytes
	
	if (addrBuffer == 0xff)
	{
		readTopAddr.var = currentAddr.var;
	}
	else
	{
		readTopAddr.splitVar[0] = addrBuffer;
		EEAddr16_u addr;
		addr.var = EEoffsets[b] + 1;
		if(!EEReadByte(&addr, &(readTopAddr.splitVar[1]))) operationErrors++;
	}
}

void ExtEememInterface::NextPoint(ControllerAxisData *x, ControllerAxisData *y, ControllerAxisData *z)
{
	uint8_t buffer[4];
	
	switch(currentMode)
	{
		case EEmReading:
		{
			if (currentAddr.var + 4 >= readTopAddr.var) Finish();
			
			if(!EERead4Bytes(&currentAddr, buffer)) operationErrors++;
			
			x->magnitude = buffer[0];
			y->magnitude = buffer[1];
			z->magnitude = buffer[2];
			
			x->direction = (ControllerDirection)(buffer[3] & 0x03);
			y->direction = (ControllerDirection)((buffer[3] >> 2) & 0x03);
			z->direction = (ControllerDirection)((buffer[3] >> 4) & 0x03);
		}
		break;
		
		case EEmSaving:
		{
			if (currentAddr.var + 6 >= EEoffsets[currentBank+1]) Finish(); // +6 to be safe and ignore last block (no chance to overshoot to next block)
			
			buffer[0] = x->magnitude;
			buffer[1] = y->magnitude;
			buffer[2] = z->magnitude;
			buffer[3] = (x->direction) | (y->direction << 2) | (z->direction << 4);
			
			if(!EEWrite4Bytes(&currentAddr, buffer)) operationErrors++;
		}
		break;
		
		default:
		Finish();
		return;
	}
	
	// Go to the next memory cell, validity of this address will be checked on next run of this method
	currentAddr.var += 4;
}

void ExtEememInterface::Finish()
{
	if(currentMode == EEmSaving && currentBank < EEnone)
	{
		// Write address of last data point to the EEPROM
		EEAddr16_u addrn;
		addrn.var = EEoffsets[currentBank];
		if(!EEWriteByte(&addrn, &(currentAddr.splitVar[0]))) operationErrors++;
		addrn.var++;
		if(!EEWriteByte(&addrn, &(currentAddr.splitVar[1]))) operationErrors++;
	}
	
	currentMode = EEmStopped;
	currentBank = EEnone;
	
	EESendStop(); // Safety stop in case something breaks
}