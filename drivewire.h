#include <cmoc.h>
#include <coco.h>

#ifndef DRIVEWIRE_H
#define DRIVEWIRE_H

#ifndef DW_BECKER
#define DW_BECKER 0
#endif

#ifndef DW_JMPCPBCK
#define DW_JMPCPBCK 0
#endif

#ifndef DW_BECKERTO	
#define DW_BECKERTO 0
#endif

#ifndef DW_ARDUINO  
#define DW_ARDUINO 0
#endif

#ifndef DW_BAUD38400  
#define DW_BAUD38400 0
#endif

#ifndef DW_NOINTMASK 
#define DW_NOINTMASK 0
#endif

#ifndef DW_H6309
#define DW_H6309 0
#endif

#ifndef DW_SY6551N
#define DW_SY6551N 0
#endif

#ifndef DW_COCO3FPGAWIFI
#define DW_COCO3FPGAWIFI 0
#endif

int dw_read(uint8_t *buf, uint16_t count);
int dw_write(uint8_t *buf, uint16_t count);
void dw_init(void);
#endif

