 
_ramstart = 0x20000000; 
_estack = 0x20004FF0; 
_minimum_stack_size = 0x100; 

MEMORY
{
	rom (rx)      : ORIGIN = 0x08000000, LENGTH = 128K
	ram (xrw)     : ORIGIN = 0x20000000, LENGTH = 20K
}

INCLUDE "stm32f10x.ld"
