/*  font4x8.h - ISO-8859-1 font for a 51x24 software text screen.

    By Pierre Sarrazin <http://sarrazip.com/>
    This file is in the public domain.
*/

//  Accompanies version 0.2.0 of hirestxt.h.

#ifndef _font4x8_h_
#define _font4x8_h_


// Characters 32 to 127 and 160 to 255.
// Only the 5 high bits of each byte are part of the glyph.
// The 3 low bits of each byte are zero.
//
extern unsigned char font4x8[1536];


#endif  /* _font4x8_h_ */
