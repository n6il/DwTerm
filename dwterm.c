/* DriveWire4 CLI and Terminal                      */
/*                                                  */
/* By Michael Furman, July 2 2017                   */
/*                                                  */
/* This file is in the public domain                */

#include <coco.h>
#include <cmoc.h>

#ifndef LITE
#include "hirestxt.h"
// #include "hirestxt.c"
#include "font4x8.h"
// #include "font4x8.c"
#include "vt100.h"
#include "width64.h"
#endif

#include "dwterm.h"
#include "drivewire.h"

// DW Commands
#define OP_SERSETSTAT 0xC4
#define OP_SERREAD 0x43
#define OP_SERREADM 0x63
#define OP_FASTWRITE 0x80
// DW SETSERSTAT Codes
#define SS_Open 0x29
#define SS_Close 0x2A

#ifdef LITE
#define vt100_puts printf
#define vt100_putchar putchar
#define vt100 putchar
#define sgrClear()
#endif

//#define DEBUG

uint8_t channel_open = 0;
#define BUFFER_SIZE 1024
uint8_t buffer[BUFFER_SIZE];
uint8_t vt100En = 0;

void dw_putb(uint8_t c)
{
	uint8_t buf[1];
	buf[0] = c;
	dw_write(buf, 1);
}

uint8_t dw_getb()
{
	uint8_t buf[1];
	int checksum;
	checksum = dw_read(buf, 1);
	if (checksum <= 0)
		return 0;
	return buf[0];
}


// open_channel
// Open a DW Serial Channel
// channel - channel to open
void open_channel(uint8_t channel)
{
	dw_putb(OP_SERSETSTAT);
	dw_putb(channel);
	dw_putb(SS_Open);
#ifdef DEBUG
	printf( "OPEN CHANNEL " );
	printf("%d", channel);
	putchar( '\r' );
#endif
	channel_open = channel;
}

// get_status
// Get channel status from DW Server
void get_status(uint8_t channel, uint8_t *status)
{
	// printf( "GET STATUS " );
	// printf("%d", channel);
	// putchar( '\r' );
	dw_putb(OP_SERREAD);
	// dw_putb(channel);

	dw_read(status, 2);

#ifdef DEBUG
	printf("%x %x\n", status[0], status[1]);
#endif
	// return (uint16_t)*buf;
}

void close_channel(uint8_t channel)
{
	uint8_t buf[2];
	dw_putb(OP_SERSETSTAT);
	dw_putb(channel);
	dw_putb(SS_Close);
	get_status(channel, buf);
}

void writeChannel(uint8_t channel, uint8_t c)
{
	dw_putb(OP_FASTWRITE+channel);
	dw_putb(c);
}

uint16_t readChannel(uint8_t channel, uint8_t *buf, uint16_t size, uint8_t wait)
{
	uint16_t read=0;
	uint8_t status[2] = {0, 0};
	get_status(channel, status);
	if (wait)
		while(status[0] == 0)
			get_status(channel, status);
	else
		if (status[0] == 0)
			return 0;
	while(status[0] != 0 && read<size)
	{
		if (status[0]<16)
			*buf++ = status[1];
		else if (status[0]>16 && status[1] > 0)
		{
			uint16_t count = (size-read)<status[1] ? size-read : (uint16_t)status[1];
			dw_putb(OP_SERREADM);
			dw_putb(channel);
			dw_putb((uint8_t)count);
			dw_read(buf, count);
			read += count;
			buf += count;
		}
		else if (status[0]==16)
			channel_open = 0;
		get_status(channel, status);
	}
	return read;
}

uint16_t get_line(uint8_t *buf)
{
	uint8_t i;
	uint16_t n=0;
	while(1)
	{
		i = inkey();
		if (i == '\r')
			break;
		else if (i != 0xff) {
			*buf++ = i;
			n++;
		}
	}
	return n;
}

#define SLEEP_FAST 5
#define SLEEP_SLOW 200
const char *prompt = "DWTERM> ";
const char *banner = "DW TERMINAL 0.2dev\r\n(GPL) MARCH 30, 2020\r\nMICHAEL FURMAN <N6IL@OCS.NET>\r\n\r\n";

void dwtrm_puts(char *s)
{
	if (vt100En)
		vt100_puts(s);
	else
		printf(s);
}

void dwtrm_putchar(char s)
{
	if (vt100En)
		vt100_putchar(s);
	else
		putchar(s);
}

int main()
{
	uint8_t i, *p;
	uint16_t n;
	uint8_t iac = 0;
	uint16_t sleep = SLEEP_SLOW;
	uint8_t brk = 0;
	uint8_t echo = 1;
	
	initCoCoSupport();
#ifndef LITE
	struct HiResTextScreenInit hrinit =
	{
	    51,
	    writeCharAt_51cols,
	    (byte) (0x0E00 / 512),
	    TRUE
	};
	if (isCoCo3)
	{
		setHighSpeed(1);
		printf("SELECT SCREEN MODE\r\r(1) COCO3 80X24 ANSI\r(2) PMODE 4 51X24 VT52\r(3) DEFAULT 32X16 ANSI");
		i=0;
		while(!i) i=inkey();
		if(i=='1') {
			width(80);
			vt100_setup(80, 24, 1, (uint8_t *)0x8000);
			vt100En = 1;
		} else if(i=='2') {
			initHiResTextScreen(&hrinit);
		}
		else if(i=='3') {
			width(32);
			vt100_setup(32, 16, 0, (uint8_t *)0x400);
			vt100En = 1;
		}
	}
	else	
	{
#ifdef LITE
		vt100En = 0;
#else
		printf("SELECT SCREEN MODE\r\r(1) COCOVGA 64X32 ANSI\r(2) PMODE 4 51X24 VT52\r(3) DEFAULT 32X16 ANSI");
		i=0;
		while(!i) i=inkey();
		if(i=='1') {
			width64();
			vt100_setup(64, 32, 0, (uint8_t *)0xe00);
			vt100En = 1;
		}
		else if(i=='2') {
			initHiResTextScreen(&hrinit);
		}
		else {
			vt100_setup(32, 16, 0, (uint8_t *)0x400);
			vt100En = 1;
		}
#endif

	}
#endif
	inkey(); // toss first key
	dwtrm_puts(banner);
	dwtrm_puts("CLOSE: BREAK-C  QUIT: BREAK-Q\r\n");
	dwtrm_puts("ECHO: BREAK-E   ^C: BREAK-BREAK\r\n");
	dwtrm_puts("TYPE DW COMMANDS AT THE PROMPT\r\n\r\n");
	
   dw_init();
	open_channel(1);
	get_status(1, buffer);
	dwtrm_puts(prompt);
	while(1)
	{
		// Get Keyboard Input
		i=inkey();
		if( i )
		{
			if (i == 3) {
				if (brk == 0)
					brk = i;
				else if (brk == 3) {
					brk = 0;
				}
			} else if (brk == 3) {
				if ( i == 'Q' || i == 'q' ) {
					dwtrm_puts("\r\nCLOSING CHANNEL\r\n");
					close_channel(1);
					dwtrm_puts("\r\nQUIT\r\n");
					break;
				} else if ( i  == 'C' || i == 'c' ) {
					dwtrm_puts("\r\nCLOSING CHANNEL\r\n");
					channel_open = 0;
					sleep = 1;
				} else if ( i  == 'E' || i == 'e' ) {
					echo ^= 0x01;
					dwtrm_puts("\r\nECHO ");
					dwtrm_puts(echo ? "ON" : "OFF");
					dwtrm_puts("\r\n");
					brk = 0;
					continue;
				} else {
					brk = 0;
				}

			} 
			if (brk == 0) {
				if (echo)
					dwtrm_putchar( i );
				writeChannel(1, i);
				sleep = SLEEP_FAST;
			}
		}
		// Make the keyboard more reactive by only polling it
		// frequently.  DW will be polled for data only when sleep
		// countdown has reached 0
		if (--sleep)
			continue;
		n = readChannel(1, buffer, BUFFER_SIZE, FALSE);
		if (n == 0)
			// Go back to polling the keyboard frequently if
			// there was no data
			sleep = SLEEP_SLOW;
		else
			// If there was some data poll DW for data more
			// quickly
			sleep = SLEEP_FAST;
		// Process received data
		for(p=buffer; n>0; n--)
		{
			i = *p++;
			if (iac) {
				writeChannel(1, 255);
				writeChannel(1, iac);
				writeChannel(1, i);
				iac = 0;
				continue;
			}
			switch(i)
			{
				// Process Telnet IAC Protocol
				case 251:
					iac = 253;
					break;
				case 252:
					iac = 254;
					break;
				case 253:
				case 254:
					iac = 252;
					break;
				case 255:
				// Console has auto newline so dump it
				// case '\n':
					break;
				// Write any other chars to the console
				default:
					if (vt100En)
						vt100(i);
					else if (i != '\n')
						putchar(i);
			}
			// if (i != '\n')
			//	putchar(i);
		}
		// Re-open the DW channel if it was closed.  DriveWire4 will
		// close the channel after interactive DW commands
		if (!channel_open)
		{
			close_channel(1);
			get_status(1, buffer);
			open_channel(1);
			//asm("emubrk");
			get_status(1, buffer);
			if (vt100En)
				sgrClear();
			dwtrm_puts("\r\n");
			dwtrm_puts(prompt);
			brk = 0;
		}
	};
	
	return 0;
}

