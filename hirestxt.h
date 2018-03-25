/*  hirestxt.h - 51x24 black-on-green PMODE 4 text screen.

    By Pierre Sarrazin <http://sarrazip.com/>
    This file is in the public domain.

    Quick Guide:
        - Call initHiResTextScreen() first.
        - End with a call to closeHiResTextScreen().
        - #define HIRESTEXT_NO_VT52 to avoid compiling the VT-52 code.
*/

/*  Version 0.1.0 - 2016-05-01 - First public release.
    Version 0.1.1 - 2016-09-12 - Adapted to CMOC 0.1.31 re: inline asm.
    Version 0.1.2 - 2016-12-26 - HIRESTEXT_NO_VT52 to avoid VT-52 code.
    Version 0.2.0 - 2017-??-?? - Adapted to modular compilation under CMOC 0.1.43.
*/

#ifndef _hirestxt_h_
#define _hirestxt_h_

#include <coco.h>

#include "font4x8.h"


enum
{
    HIRESWIDTH                    = 51,     // number of text columns
    HIRESHEIGHT                   = 24,     // number of text lines
    PIXEL_COLS_PER_SCREEN         = 256,
    PIXEL_ROWS_PER_SCREEN         = 192,
    PIXEL_ROWS_PER_TEXT_ROW       = 8,
    BYTES_PER_PIXEL_ROW           = 32,
};


// Text cursor position.
// textPosX is allowed to be equal to HIRESWIDTH. When this many characters
// have been written on a line, the cursor is considered to be not on the
// next line, but past the last displayed column on the original line.
// Then when a '\n' is written, the cursor goes to column 0 of the next line.
// This avoids having the '\n' insert an empty line.
// See how '\n' is processed by writeChar().
//
// To change this position, call moveCursor().
//
extern byte textPosX;  // 0..HIRESWIDTH
extern byte textPosY;  // 0..HIRESHEIGHT-1

// Location of the 6k PMODE 4 screen buffer.
// Must be a multiple of 512.
//
extern byte *textScreenBuffer;


void moveCursor(byte x, byte y);
void setTextScreenAddress(byte pageNum);

#ifndef HIRESTEXT_NO_VT52
void initVT52();
#endif


// Call this first.
//
// textScreenPageNum: 512-byte page index in 0..127.
// Actual graphics address becomes textScreenPageNum * 512.
// To position the screen at $0E00, divide this address by 512,
// which gives 7.
//
// The screen is cleared to green. The font is black on green.
//
// If redirectPrintf is true, printf() will print to the
// hi-res screen until closeHiResTextScreen() is called.
// This option has no effect if HIRESTEXT_NO_VT52 is #defined.
//
void initHiResTextScreen(byte textScreenPageNum, byte redirectPrintf);


// Call this last.
//
void closeHiResTextScreen();


// Clear text rows from y (0..HIRESHEIGHT-1) to the end of the screen.
// Does nothing if y is out of range.
//
void clearRowsToEOS(byte byteToClearWith, byte textRow);


// pageNum: 512-byte page index in 0..127.
// Actual graphics address becomes pageNum * 512.
//
void setTextScreenAddress(byte pageNum);


// Writes a PRINTABLE 4x8 character at column x and row y of a 51x24 text screen.
// x: 0..HIRESWIDTH-1.
// y: 0..HIRESHEIGHT-1.
// asciiCode: MUST be in the range 32..127 or 160..255, except if 0,
// which means invert colors at position (x, y).
// Uses textScreenBuffer, frameByteAddrTable[], frameBitOffsetTable[] and
// frameMaskTable[].
//
// Does NOT advance the cursor.
//
void writeCharAt(byte x, byte y, byte asciiCode);


void invertPixelsAtCursor();


// Scrolls the 51x24 screen one text row up.
// Fills the bottom text row with set pixels.
//
void scrollTextScreenUp();


// x: Column number (0..HIRESWIDTH-1).
// y: Row number (0..HIRESHEIGHT-1).
// Does nothing if x or y are out of range.
//
void moveCursor(byte x, byte y);


// Puts the cursor in the upper left position.
//
#define home() (moveCursor(0, 0))


// Fills the screen with spaces (does not move the cursor).
//
#define clear() (clearRowsToEOS(0xff, 0))


// Homes the cursor and clears the screen.
//
#define clrscr() do { home(); clear(); } while (0)


// Writes a character at the current cursor position and advances the cursor.
// Scrolls the screen up one row if the cursor would go off the bottom.
// Ignores non-printable characters.
// str: Supports \a, \b, \t, \n, \f, \r.
//
void writeChar(byte ch);


// Calls writeChar() for each character in the given string,
// up to and excluding the terminating '\0'.
//
void writeString(char *str);


// row: 0..HIRESHEIGHT-1
// line: String of at most HIRESWIDTH characters.
// Leaves the cursor at the end of the line.
//
void writeCenteredLine(byte row, char *line);


// Writes unsigned word 'w' to the screen in decimal.
//
void writeDecWord(word w);

// Writes spaces from the cursor to the end of the current line.
// Does NOT move the cursor.
//
void clrtoeol();


// Writes spaces from the cursor to the end of the screen.
// Does NOT move the cursor.
//
void clrtobot();


// Removes the cursor if present (it is typically displayed
// by animateCursor()).
//
void removeCursor();


// To be called periodically.
// Call removeCursor() when the animation must stop.
//
void animateCursor();


// Returns ASCII code of pressed key (using inkey()).
// Uses textPosX, textPosY.
//
byte waitKeyBlinkingCursor();


///////////////////////////////////////////////////////////////////////////////


#ifndef HIRESTEXT_NO_VT52

// Resets the VT52 state machine.
//
void initVT52();


#endif  /* HIRESTEXT_NO_VT52 */


// Calls writeChar() and moveCursor() to execute the VT52 sequence
// provided by the calls to this function.
//
// Source: http://bitsavers.trailing-edge.com/pdf/dec/terminal/gigi/EK-0GIGI-RC-001_GIGI_Programming_Reference_Card_Sep80.pdf
//
void processConsoleOutChar(byte ch);


#endif  /* _hirestxt_h_ */
