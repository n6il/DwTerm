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


//#define DEBUG

uint8_t channel_open = 0;
#define BUFFER_SIZE 1024
uint8_t buffer[BUFFER_SIZE];

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
const char *banner = "DW TERMINAL 0.1\r(GPL) MARCH 25, 2018\rMICHAEL FURMAN <N6IL@OCS.NET>\r\r";

int main()
{
	uint8_t i, *p;
	uint16_t n;
	uint8_t iac = 0;
	uint16_t sleep = SLEEP_SLOW;
	
	initCoCoSupport();
#ifndef LITE
	if (isCoCo3)
	{
		setHighSpeed(1);
		printf("SELECT SCREEN MODE\r\r(1) COCO3 80X24\r(2) PMODE 4 51X24 SCREEN\r(3) DEFAULT 32X16 SCREEN");
		i=0;
		while(!i) i=inkey();
		if(i=='1')
			width(80);
		else if(i=='2')
			initHiResTextScreen(0x0E00 / 512, TRUE);
		else if(i=='3')
			width(32);
	}
	else	
	{
		printf("SELECT SCREEN MODE\r\r(1) PMODE 4 51X24 SCREEN\r(2) DEFAULT 32X16 SCREEN");
		i=0;
		while(!i) i=inkey();
		if(i=='1')
			initHiResTextScreen(0x0E00 / 512, TRUE);
	}
#endif
	inkey(); // toss first key
	printf("%s", banner);
	printf("TYPE DW COMMANDS AT THE PROMPT\r\r");
	
   dw_init();
	open_channel(1);
	get_status(1, buffer);
	printf("%s", prompt);
	while(1)
	{
		// Get Keyboard Input
		i=inkey();
		if( i )
		{
			putchar( i );
			writeChannel(1, i);
			sleep = SLEEP_FAST;
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
				case '\n':
					break;
				// Write any other chars to the console
				default:
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
			printf( "\r%s", prompt);
		}
	};
	
	return 0;
}

