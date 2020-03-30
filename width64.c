#include <cmoc.h>
#include <coco.h>
#include "width64.h"

/*
 *
 * The following is copied from:
 *
 * http://www.cocovga.com/documentation/software-mode-control/
 *
 */

#define WIDTH64ADDR 0x2000
#define WIDTH64DLEN 9
uint8_t width64_reg_data[WIDTH64DLEN] = {
    0x00, // reset register - reset no register banks
    0x81, // edit mask - modify enhanced modes and font registers
    0x00, // reserved
    0x02, // font - force lowercase (use #$03 for both lowercase and T1 character set)
    0x00, // artifact
    0x00, // extras
    0x00, // reserved
    0x00, // reserved
    0x02, // enhanced modes - 64-column enable
};

void width64(void)
{
	// 1.  Set up a 512-byte-aligned memory region with register values you want to stream into CoCoVGA. For this example, 
	// we are arbitrarily selecting address $2000 as the start address.
	memcpy((void *)WIDTH64ADDR, (const void*)width64_reg_data, WIDTH64DLEN);

	asm {
* 2. Wait for VSYNC, which signals the start of the vertical blanking region.

    PSHS CC     save CC
    ORCC #$50   mask interrupts

    LDA  $FF03 
    PSHS A      save PIA configuration 

    LDA  $FF03
    ORA  #$04   ensure PIA 0B is NOT setting direction
    STA  $FF03

    LDA  $FF03 
    ANDA #$FD   vsync flag - trigger on falling edge
    STA  $FF03

    LDA  $FF03
    ORA  #$01   enable vsync IRQ (although interrupt itself will be ignored via mask)
    STA  $FF03

    LDA  $FF02  clear any pending VSYNC
@L1 LDA  $FF03  wait for flag to indicate...
    BPL  @L1    ...falling edge of FS (Field Sync)
    LDA  $FF02  clear VSYNC interrupt flag

* 3. During VSYNC, point SAM to 512 byte page set up in step 1 (via SAM page select registers $FFC6-$FFD3). For this
* example, this page is at $7C00. Divide by 512 to get page number:
*     $FC00/512 = $3E = 011 1110
* SAM page selection is performed by writing a single address to set or clear each bit. In this case we want to 
* clear bits 0 and 6, so write to even addresses for those, and write to odd addresses to set bits 1 through 5.

*    STA  $FFC6  clear SAM_F0
*    STA  $FFC9  set SAM_F1
*    STA  $FFCB  set SAM_F2
*    STA  $FFCD  set SAM_F3
*    STA  $FFCF  set SAM_F4
*    STA  $FFD1  set SAM_F5
*    STA  $FFD2  clear SAM_F6

*   $2000 / 512 = $10 = 001 0000
    STA  $FFC6  clear SAM_F0
    STA  $FFC8  clear SAM_F1
    STA  $FFCA  clear SAM_F2
    STA  $FFCC  clear SAM_F3
    STA  $FFCF  set SAM_F4
    STA  $FFD0  clear SAM_F5
    STA  $FFD2  clear SAM_F6

* 4. Also during VSYNC, write 6847 VDG combination lock via PIA1B ($FF22) bits 7:3.

    LDA  $FF22  get current PIA value
    ANDA  #$07  mask off bits to change
    ORA   #$90  set combo lock 1
    STA  $FF22  write to PIA
    ANDA  #$07  mask off bits to change
    ORA   #$48  set combo lock 2
    STA  $FF22  write to PIA
    ANDA  #$07  mask off bits to change
    ORA   #$A0  set combo lock 3
    STA  $FF22  write to PIA
    ANDA  #$07  mask off bits to change
    ORA   #$F8  set combo lock 4
    STA  $FF22  write to PIA

* 5. Still during VSYNC, configure VDG and SAM back to mode 0. (In this case, the desired CoCoVGA register page to program is 0.)

    LDA  $FF22  get current PIA value
    ANDA  #$07  mask off bits to change
    ORA   #$00  select CoCoVGA register page 0
    STA  $FF22  write to PIA
    STA  $FFC0  clear SAM_V0
    STA  $FFC2  clear SAM_V1
    STA  $FFC4  clear SAM_V2

* 6. Wait for next VSYNC while SAM and VDG stream in the entire page of register values to CoCoVGA and CoCoVGA 
* displays the previous frame of video.

@L2 LDA  $FF03  wait for flag to indicate...
    BPL  @L2    ...falling edge of FS (Field Sync)
    LDA  $FF02  clear VSYNC interrupt flag
    PULS A      from stack... 
    STA  $FF03  ...restore original PIA configuration
    PULS CC     restore ability to see interrupts

* 7. Point SAM page select to video page you want to display. For this example, lets assume that this is at $E00
* which (divided by 512 bytes) is page 7.

    STA  $FFC7  set SAM_F0
    STA  $FFC9  set SAM_F1
    STA  $FFCB  set SAM_F2
    STA  $FFCC  clear SAM_F3
    STA  $FFCE  clear SAM_F4
    STA  $FFD0  clear SAM_F5
    STA  $FFD2  clear SAM_F6

* 8. Program SAM and VDG to the appropriate video mode. As the final gate to enabling 64-column text mode, CoCoVGA
* recognizes the VDGs only 2kB mode, CG2 ($2).

    LDA  $FF22  get current PIA value
    ANDA  #$0F  mask off bits to change
    ORA   #$A0  set VDG to CG2
    STA  $FF22  write to PIA
    STA  $FFC0  clear SAM_V0
    STA  $FFC3  set SAM_V1
    STA  $FFC4  clear SAM_V2
  }
} 
