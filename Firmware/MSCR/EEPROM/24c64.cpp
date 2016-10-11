/*
* Copyright 2014 Marcin Szeniak
*/

#include <avr/io.h>
#include <util/delay.h>

#include "24c64.h"
#include <util/twi.h>

void EESendStop()
{
	//Put Stop Condition on bus
	TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	
	//Wait for STOP to finish
	//while(TWCR & (1<<TWSTO));
}

uint8_t EESetAddr(EEAddr16_u *address)
{
	do
	{
		//Put Start Condition on TWI Bus
		TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

		//Poll Till Done
		while(!(TWCR & (1<<TWINT)));

		//Check status
		if((TWSR & 0xF8) != TW_START)
		return FALSE;

		//Now write SLA+W
		//EEPROM @ 00h
		TWDR=0xa0; //0b10100000; // Control code 1010 for chip select, 000 for chip address, 0 for write

		//Initiate Transfer
		TWCR=(1<<TWINT)|(1<<TWEN);

		//Poll Till Done
		while(!(TWCR & (1<<TWINT)));
		
	}while((TWSR & 0xF8) != 0x18);
	

	//Now write ADDRH
	TWDR=(address->splitVar[1]);

	//Initiate Transfer
	TWCR=(1<<TWINT)|(1<<TWEN);

	//Poll Till Done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_MT_DATA_ACK)
	return FALSE;

	//Now write ADDRL
	TWDR=(address->splitVar[0]);

	//Initiate Transfer
	TWCR=(1<<TWINT)|(1<<TWEN);

	//Poll Till Done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_MT_DATA_ACK)
		return FALSE;
		
	return TRUE;
}

uint8_t EEGoToAddr(EEAddr16_u *address)
{
	if(!EESetAddr(address))
	return FALSE;
	
	//Put Stop Condition on bus
	TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	
	//Wait for STOP to finish
	while(TWCR & (1<<TWSTO));
	
	return TRUE;
}

uint8_t EEReadRelative (uint8_t*output)
{
	//Put Start Condition on TWI Bus
	TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//Poll Till Done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_REP_START && (TWSR & 0xF8) != TW_START)
	return FALSE;

	//Now write SLA+R
	//EEPROM @ 00h
	TWDR=0xa1; //0b10100001; // Control code 1010 for chip select, 000 for chip address, 1 for read

	//Initiate Transfer
	TWCR=(1<<TWINT)|(1<<TWEN);

	//Poll Till Done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_MR_SLA_ACK)
	return FALSE;

	//Now enable Reception of data by clearing TWINT
	TWCR=(1<<TWINT)|(1<<TWEN);

	//Wait till done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_MR_DATA_NACK)
	return FALSE;

	//Read the data
	*output=TWDR;

	//Put Stop Condition on bus
	TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	
	//Wait for STOP to finish
	while(TWCR & (1<<TWSTO));

	//Return TRUE
	return TRUE;
}

uint8_t EEWriteByte(EEAddr16_u *address,uint8_t *data)
{
	if(!EESetAddr(address))
		return FALSE;

	//Now write DATA
	TWDR=(*data);

	//Initiate Transfer
	TWCR=(1<<TWINT)|(1<<TWEN);

	//Poll Till Done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_MT_DATA_ACK)
		return FALSE;

	//Put Stop Condition on bus
	TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	
	//Wait for STOP to finish
	while(TWCR & (1<<TWSTO));

	//Wait untill Writing is complete
	_delay_ms(5);

	//Return TRUE
	return TRUE;

}

uint8_t EEReadByte(EEAddr16_u *address, uint8_t*output)
{
	if(!EESetAddr(address))
		return FALSE;

	if(!EEReadRelative(output))
		return FALSE;
		
	return TRUE;
}

uint8_t EEWrite4Bytes(EEAddr16_u *address, uint8_t data[4])
{
	if(!EESetAddr(address))
		return FALSE;

	for(uint8_t i = 0; i < 4; i++)
	{
		//Now write DATA
		TWDR=data[i];

		//Initiate Transfer
		TWCR=(1<<TWINT)|(1<<TWEN);

		//Poll Till Done
		while(!(TWCR & (1<<TWINT)));
	}
	
	//Check status
	//if((TWSR & 0xF8) != TW_MT_DATA_ACK)
	//	return FALSE;

	//Put Stop Condition on bus
	TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	
	//Wait for STOP to finish
	while(TWCR & (1<<TWSTO));

	//Wait until Writing is complete
	//_delay_ms(12);

	return TRUE;
}

uint8_t EERead4BytesRelative(uint8_t output[4])
{
	//Put Start Condition on TWI Bus
	TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//Poll Till Done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_REP_START && (TWSR & 0xF8) != TW_START)
		return FALSE;
	
	//Now write SLA+R
	//EEPROM @ 00h
	TWDR=0xa1; //0b10100001; // Control code 1010 for chip select, 000 for chip address, 1 for read

	//Initiate Transfer
	TWCR=(1<<TWINT)|(1<<TWEN);

	//Poll Till Done
	while(!(TWCR & (1<<TWINT)));

	//Check status
	if((TWSR & 0xF8) != TW_MR_SLA_ACK)
	return FALSE;

	TWCR=(1<<TWINT)|(1<<TWEA)|(1<<TWEN); //Now enable Reception of data by clearing TWINT
	while(!(TWCR & (1<<TWINT))); //Wait till done
	if((TWSR & 0xF8) != TW_MR_DATA_ACK) return FALSE; //Check status
	output[0]=TWDR; //Read the data
	
	TWCR=(1<<TWINT)|(1<<TWEA)|(1<<TWEN); //Now enable Reception of data by clearing TWINT
	while(!(TWCR & (1<<TWINT))); //Wait till done
	if((TWSR & 0xF8) != TW_MR_DATA_ACK) return FALSE; //Check status
	output[1]=TWDR; //Read the data
	
	TWCR=(1<<TWINT)|(1<<TWEA)|(1<<TWEN); //Now enable Reception of data by clearing TWINT
	while(!(TWCR & (1<<TWINT))); //Wait till done
	if((TWSR & 0xF8) != TW_MR_DATA_ACK) return FALSE; //Check status
	output[2]=TWDR; //Read the data
	
	TWCR=(1<<TWINT)|(1<<TWEN); //Now enable Reception of data by clearing TWINT
	while(!(TWCR & (1<<TWINT))); //Wait till done
	if((TWSR & 0xF8) != TW_MR_DATA_NACK) return FALSE; //Check status
	output[3]=TWDR; //Read the data

	//Put Stop Condition on bus
	TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	
	//Wait for STOP to finish
	while(TWCR & (1<<TWSTO));

	//Return TRUE
	return TRUE;
}

uint8_t EERead4Bytes(EEAddr16_u *address, uint8_t output[4])
{
	if(!EESetAddr(address))
	return FALSE;
	
	if(!EERead4BytesRelative(output))
	return FALSE;
	
	return TRUE;
}