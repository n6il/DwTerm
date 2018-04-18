#include <cmoc.h>
#include <coco.h>

#include "drivewire.h"

asm void dw_init(void) {
   asm
   {
		* DW Defines
BECKER		EQU DW_BECKER
JMCPBCK		EQU DW_JMPCPBCK
BECKERTO	EQU DW_BECKERTO
ARDUINO		EQU DW_ARDUINO
BAUD38400	EQU DW_BAUD38400
NOINTMASK	EQU DW_NOINTMASK
H6309		EQU DW_H6309
SY6551N		EQU DW_SY6551N
COCO3FPGAWIFI	EQU DW_COCO3FPGAWIFI

		use "dwdefs.d"
IntMasks	EQU $50
* BBIN        	equ    $FF22
* BBOUT       	equ    $FF20
* BCKCTRL        	equ    $FF41
* BCKDATA       	equ    $FF42

      lbra dwinitbegin
      use "dwinit.asm"
dwinitbegin
      lbsr DWInit
   }
}

asm int dw_read(uint8_t *buf, uint16_t count)
{
	asm
	{

		lbra dwrbegin
		use "dwread.asm"
dwrbegin
		pshs x,y
		ldx 6,s		buf
		ldy 8,s		count
		lbsr DWRead
		bne allread@
		bcs frameerr@
		clrd		nothing read
		bra done@
frameerr@
		ldd #-1		framing error
		bra done@
allread@
		tfr y,d		checksum
done@
		puls x,y
	}
}

asm int dw_write(uint8_t *buf, uint16_t count)
{
	asm
	{
		lbra dwwbegin
		use "dwwrite.asm"
dwwbegin
		* DW Defines above in dw_read
		pshs x,y
		ldx 6,s		buf
		ldy 8,s		count
		lbsr DWWrite
		tfr x,d
		subd 2,s	buf
		decd
		puls x,y
	}
}
