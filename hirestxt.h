/*  hirestxt.h - 51x24 black-on-green PMODE 4 text screen.

    By Pierre Sarrazin <http://sarrazip.com/>
    This file is in the public domain.

    Quick Guide:
        - Call initHiResTextScreen() first.
        - End with a call to closeHiResTextScreen().
        - #define HIRESTEXT_NO_VT52 to avoid compiling the VT-52 code.
*/

#ifndef _hirestxt_h_
#define _hirestxt_h_

#include <coco.h>


// Writes a character at position (x, y) on a 42x24 text screen.
//
void writeCharAt_42cols(byte x, byte y, byte asciiCode);


// Writes a character at position (x, y) on a 42x24 text screen.
//
void writeCharAt_51cols(byte x, byte y, byte asciiCode);


// Pointer to a function that writes a character at position (x, y).
//
typedef void (*WriteCharAtFuncPtr)(byte x, byte y, byte asciiCode);


// Initializer to be used when calling initHiResTextScreen().
//
// numColumns: 42 or 51 (characters per line).
//
// writeCharAtFuncPtr: Either writeCharAt_42cols or writeCharAt_51cols.
//                     Must be consistent with numColumns.
//
// textScreenPageNum: 512-byte page index in 0..127.
// Actual graphics address becomes textScreenPageNum * 512.
// To position the screen at $0E00, divide this address by 512,
// which gives 7.
//
// If redirectPrintf is true, printf() will print to the
// hi-res screen until closeHiResTextScreen() is called.
// This option has no effect if HIRESTEXT_NO_VT52 is #defined.
//
struct HiResTextScreenInit
{
    byte numColumns;
    WriteCharAtFuncPtr writeCharAtFuncPtr;
    byte textScreenPageNum;
    BOOL redirectPrintf;
};


enum
{
    HIRESHEIGHT                   = 24,     // number of text lines
    PIXEL_COLS_PER_SCREEN         = 256,
    PIXEL_ROWS_PER_SCREEN         = 192,
    PIXEL_ROWS_PER_TEXT_ROW       = 8,
    BYTES_PER_PIXEL_ROW           = 32,
};


// Width in characters of the high-resolution text screen.
//
extern byte hiResWidth;

// Text cursor position.
// textPosX is allowed to be equal to hiResWidth. When this many characters
// have been written on a line, the cursor is considered to be not on the
// next line, but past the last displayed column on the original line.
// Then when a '\n' is written, the cursor goes to column 0 of the next line.
// This avoids having the '\n' insert an empty line.
// See how '\n' is processed by writeChar().
//
// To change this position, call moveCursor().
//
extern byte textPosX;  // 0..hiResWidth
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


// Call this first, with a non null pointer to an initializer struct.
//
// See struct HiResTextScreenInit above.
//
// Does not keep a reference of the HiResTextScreenInit object.
//
// The screen is cleared to green. The font is black on green.
//
void initHiResTextScreen(struct HiResTextScreenInit *init);


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
// x: 0..hiResWidth-1.
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


// x: Column number (0..hiResWidth-1).
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
void writeString(const char *str);


// row: 0..HIRESHEIGHT-1
// line: String of at most hiResWidth characters.
// Leaves the cursor at the end of the line.
//
void writeCenteredLine(byte row, const char *line);


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


// Returns ASCII code of pressed key (using inkey(), which calls Color Basic).
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
