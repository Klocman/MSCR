/*
* Copyright 2014 Marcin Szeniak
*/

#include "aux_globals.h"

void delay_int(unsigned long delay)
{
	while(delay--) asm volatile("nop");
}