#include <cmoc.h>
#include <coco.h>
#include "coco3.h"

uint8_t palette[8] = {
	0x00, // Black
	0x24, // Red
	0x12, // Green
	0x36, // Brown
	0x09, // Blue
	0x2d, // Magenta
	0x1b, // Cyan
	0x3f  // White
};

void setupPalette(void)
{
	int i;
	char *p = (char *)0xffb0;

	// Background (0-7)
	for (i=0; i<8; i++)
		*p++ = palette[i];
	// Foreground (8-15)
	for (i=0; i<8; i++)
		*p++ = palette[i];
}
