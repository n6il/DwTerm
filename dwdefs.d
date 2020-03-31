********************************************************************
* dwdefs - DriveWire Definitions File
*
* $Id: dwdefs.d,v 1.10 2010/02/21 06:24:47 aaronwolfe Exp $
*
* Ed.    Comments                                       Who YY/MM/DD
* ------------------------------------------------------------------
*   1    Started                                        BGP 03/04/03
*   2    Added DWGLOBS area                             BGP 09/12/27

         nam   dwdefs
         ttl   DriveWire Definitions File

 IFNDEF DWDEFS_D
DWDEFS_D    equ    1

* Addresses
BBOUT       equ    $FF20
BBIN        equ    $FF22
 IFNE COCO3FPGAWIFI
BCKCTRL     equ    $FF6C
BCKDATA     equ    $FF6D
 ELSE
BCKCTRL     equ    $FF41
BCKDATA     equ    $FF42
 ENDC

MPIREG      equ    $FF7F        MPI Register
CTSMASK     equ    $F0          Get CTS
CTSSHIFT    equ    $4           Number of shifts for CTS
SCSMASK     equ    $0F          Get SCS

 IFNE MEGAMINIMPI
MMMSLT      equ    $05          MPI Slot(SCS) for MegaMiniMPI
MMMU1A      equ    $FF40        Address for UART 1
MMMU2A      equ    $FF50        Address for UART 2 
THR         equ    $00          Transmit Holding Register
RHR         equ    $00          Recieve Holding Resister
IER         equ    $01          Interrupt Enable Register
IIR         equ    $02          Interrupt Identification Register
FCR         equ    $02          FIFO Control Register
LCR         equ    $03          Line Control Register
MCR         equ    $04          Modem Control Register
LSR         equ    $05          Line Status Register
MSR         equ    $06          Modem Status Register
SCR         equ    $07          Scratch Register
RESET       equ    $08          Reset
DLL         equ    $00          Divisor Latch LSB
DLM         equ    $01          Divisor Latch MSB
DL16        equ    $0A          16-bit divisor window

* 16550 Line Control Register
LCR5BIT     equ    %00000000
LCR6BIT     equ    %00000001
LCR7BIT     equ    %00000010
LCR8BIT     equ    %00000011
LCRPARN     equ    %00000000
LCRPARE     equ    %00000100
LCRPARO     equ    %00001100
* BREAK
BRKEN       equ    %01000000
BRKDIS      equ    %10111111
* 16550 DLAB
DLABEN      equ    %10000000
DLABDIS     equ    %01111111

* 16550 Baud Rate Definitions
B600        equ    3072
B1200       equ    1536
B2400       equ    768
B4800       equ    384
B9600       equ    192
B19200      equ    96
B38400      equ    48
B57600      equ    32
B115200     equ    16
B230400     equ    8
B460800     equ    4
B921600     equ    2
B1843200    equ    1

* 16550 Line Status Register Defs
LSRDR       equ    %00000001    LSR:Data Ready
LSRTHRE     equ    %00100000    LSR:Transmit Holding Register/FIFO Empty
LSRTE       equ    %01000000    LSR:Transmit Empty

* 16550 Fifo Control Register
FCRFEN     equ    %00000001    Enable RX and TX FIFOs
FCRFDIS    equ    %11111110    Disable RX and TX FIFOs
FCRRXFCLR  equ    %00000010    Clear RX FIFO
FCRTXFCLR  equ    %00000100    Clear TX FIFO
FCRTRG1B   equ    %00000000    1-Byte FIFO Trigger
FCRTRG4B   equ    %01000000    4-Byte FIFO Trigger
FCRTRG8B   equ    %10000000    8-Byte FIFO Trigger
FCRTRG14B  equ    %11000000    14-Byte FIFO Trigger

* 16550 Modem Control Register
MCRDTREN   equ    %00000001    Enable DTR Output
MCRDTRDIS  equ    %11111110    Disable DTR Output
MCRRTSEN   equ    %00000010    Enable RTS Output
MCRRTSDIS  equ    %11111101    Disable RTS Output
MCRAFEEN   equ    %00100000    Enable Auto Flow Control
MCRAFEDIS  equ    %11011111    Disable Auto Flow Control
 ENDC

* Opcodes
OP_NOP      equ    $00		No-Op
OP_RESET1   equ    $FE		Server Reset
OP_RESET2   equ    $FF		Server Reset
OP_RESET3   equ    $F8		Server Reset
OP_DWINIT	equ	   'Z		DriveWire dw3 init/OS9 boot
OP_TIME     equ    '#	 	Current time requested
OP_INIT     equ    'I		Init routine called
OP_READ     equ    'R		Read one sector
OP_REREAD   equ    'r		Re-read one sector
OP_READEX   equ    'R+128	Read one sector
OP_REREADEX equ    'r+128	Re-read one sector
OP_WRITE    equ    'W		Write one sector
OP_REWRIT   equ    'w		Re-write one sector
OP_GETSTA   equ    'G		GetStat routine called
OP_SETSTA   equ    'S		SetStat routine called
OP_TERM     equ    'T		Term routine called
OP_SERINIT  equ    'E
OP_SERTERM  equ    'E+128

* Printer opcodes
OP_PRINT    equ    'P		Print byte to the print buffer
OP_PRINTFLUSH equ  'F		Flush the server print buffer

* Serial opcodes
OP_SERREAD equ 'C
OP_SERREADM equ 'c
OP_SERWRITE equ 'C+128
OP_SERGETSTAT equ 'D
OP_SERSETSTAT equ 'D+128

* for dw vfm
OP_VFM equ 'V+128

* WireBug opcodes (Server-initiated)
OP_WIREBUG_MODE  equ   'B
* WireBug opcodes (Server-initiated)
OP_WIREBUG_READREGS   equ  'R	Read the CoCo's registers
OP_WIREBUG_WRITEREGS  equ  'r	Write the CoCo's registers
OP_WIREBUG_READMEM    equ  'M	Read the CoCo's memory
OP_WIREBUG_WRITEMEM   equ  'm	Write the CoCo's memory
OP_WIREBUG_GO         equ  'G	Tell CoCo to get out of WireBug mode and continue execution

* VPort opcodes (CoCo-initiated)
OP_VPORT_READ         equ  'V
OP_VPORT_WRITE        equ  'v

* Error definitions
E_CRC      equ   $F3            Same as NitrOS-9 E$CRC

 ENDC
