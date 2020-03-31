#ifdef _CMOC_VERSION_
#include <coco.h>
#endif

#ifndef VT100_H
#define VT100_H

void printline(char *, int);
void move_cursor(uint8_t, uint8_t);
void erase_to_here();
void erase_from_here();
void erase_line_from_here();
void erase_line_to_here();
void clear_line();

extern uint8_t vt100buf[16];

void vt100_init(void);
void vt100_setup(uint8_t, uint8_t, uint8_t, uint8_t *);
int vt100(char);
void vt100_putchar(char);
void vt100_puts(char *);
void vt100_putstr(char *, size_t);
void sgrClear(void);

#endif

