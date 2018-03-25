/*  hirestxt.c - 51x24 black-on-green PMODE 4 text screen.

    By Pierre Sarrazin <http://sarrazip.com/>
    This file is in the public domain.

    Quick Guide:
        - Call initHiResTextScreen() first.
        - End with a call to closeHiResTextScreen().
        - #define HIRESTEXT_NO_VT52 to avoid compiling the VT-52 code.
*/

//  Accompanies version 0.2.0 of hirestxt.h.

#include "hirestxt.h"


byte textPosX;  // 0..HIRESWIDTH
byte textPosY;  // 0..HIRESHEIGHT-1
byte *textScreenBuffer;
byte hiResTextCursorPresent;
void *oldCHROOT;


void initHiResTextScreen(byte textScreenPageNum, byte redirectPrintf)
{
    initCoCoSupport();
    width(32);  // PMODE graphics will only appear from 32x16 (does nothing on CoCo 1&2)
    moveCursor(0, 0);
    setTextScreenAddress(textScreenPageNum);  // DECB's first PMODE 4 screen (7 * 512 = 0x0E00)
    pcls(textScreenBuffer, 255);
    showGraphicsAddress(textScreenPageNum);  // set SAM registers to show 0x0E00
    showPmode4(0);  // green/black
    #ifndef HIRESTEXT_NO_VT52
    initVT52();
    #endif
    hiResTextCursorPresent = FALSE;
    if (redirectPrintf)
        oldCHROOT = setConsoleOutHook(hiResTextConsoleOutHook);
    else
        oldCHROOT = 0;
}


void closeHiResTextScreen()
{
    if (oldCHROOT)
        setConsoleOutHook(oldCHROOT);
}


void processConsoleOutChar(byte ch);
void writeString(char *str);


asm void clearRowsToEOS(byte byteToClearWith, byte textRow)
{
    // Start of cleared buffer: textScreenBuffer + ((word) textRow * 32 * 8);

    // Performance notes:
    // std ,x++: 8 cycles.
    // pshu b,a: 7 cycles.
    // pshu y,b,a: 9 cycles to write 4 bytes, i.e., 4.5 cycles per 2 bytes.
    // To push 32 bytes: 72 cycles.

    asm
    {
        // Do nothing if textRow invalid.
        lda     5,s                     // textRow
        cmpa    #HIRESHEIGHT
        bhs     clearRowsToEOS_end

        // Iteration will go from end of screen buffer to start of target row.
        // Compute address of start of target row.
        clrb                            // D = textRow * 32 bytes per pixel row
                                        //             * 8 pixel rows per text row
        addd    textScreenBuffer

        pshs    u,b,a                   // put address in stack for loop, preserve U

        // Compute address of end of screen buffer. Put it in U, to use PSHU.
        ldu     textScreenBuffer
        leau    PIXEL_COLS_PER_SCREEN*PIXEL_ROWS_PER_SCREEN/8,u

        lda     7,s                     // byteToClearWith (mind the PSHS)
        tfr     a,b
        tfr     d,x                     // D & X contain 4 copies of byte to clear with
clearRowsToEOS_loop:
        pshu    x,b,a                   // decrement U by 4, write 4 bytes at U
        pshu    x,b,a                   // repeat enough to write 64 bytes per iteration
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        pshu    x,b,a
        cmpu    ,s                      // has U reached start of target row?
        bhi     clearRowsToEOS_loop     // loop if not

        puls    a,b,u,pc                // restore CMOC's stack frame pointer in U
clearRowsToEOS_end:
    }
}


void setTextScreenAddress(byte pageNum)
{
    textScreenBuffer = (byte *) ((word) pageNum << 9);
}


// Column  Byte    Bit     Mask 1   Mask 2
//
// 0       0       0       00000111 11111111
// 1       0       5       11111000 00111111
// 2       1       2       11000001 11111111
// 3       1       7       11111110 00001111
// 4       2       4       11110000 01111111
// 5       3       1       10000011 11111111
// 6       3       6       11111100 00011111
// 7       4       3       11100000 11111111
//
// A frame is a 5-byte (40-bit) region.
// In 51x24 mode, 8 characters (5 pixels wide each) fit in a frame.


// frameByteAddrTable[c] is the byte offset in a frame of
// the c-th (0..7) character in that frame.
// frameBitOffsetTable[c] is the number of right shifts to apply
// to position bits at the c-th (0..7) character of a frame.
// frameMaskTable[c] is the AND mask that applies at the c-th (0..7)
// character of a frame.
//
byte frameByteAddrTable[8]  = { 0, 0, 1, 1, 2, 3, 3, 4 };
byte frameBitOffsetTable[8] = { 0, 5, 2, 7, 4, 1, 6, 3 };
word frameMaskTable[8] = { 0x07ff, 0xf83f, 0xc1ff, 0xfe0f, 0xf07f, 0x83ff, 0xfc1f, 0xe0ff };


// Called by PUTCHR, which is called by printf() et al.
// The character to print is in A.
// It is sent to writeChar() (the 51x24 screen) instead of
// the regular CoCo screen.
//
// MUST preserve B, X, U and Y.
//
void asm hiResTextConsoleOutHook()
{
    asm
    {
        pshs    x,b

        // CMOC's printf() converts \n to \r for Color Basic's PUTCHR,
        // but processConsoleOutChar() expects \n to mean newline.
        //
        cmpa    #13
        bne     @notCR
        lda     #10
@notCR:
        pshs    a
        clr     ,-s     // push argument as a word
        lbsr    processConsoleOutChar
        leas    2,s

        puls    b,x
    }
}


void writeCharAt(byte x, byte y, byte asciiCode)
{
#if 0 // Original (tested) code in CMOC:
    byte frameCol = x % 8;
    word *screenWord = (word *) (textScreenBuffer + y * 256 + x / 8 * 5 + frameByteAddrTable[frameCol]);
        // 256 = 8 rows per character, times 32 bytes per pixel row

    // In charBitmask, only the high 5 bits of each byte are significant.
    // The others must be 0.
    //
    byte *charBitmask = font4x8 + (((word) asciiCode - (asciiCode < 128 ? 32 : 64)) << 3);

    for (byte row = 0; row < PIXEL_ROWS_PER_TEXT_ROW; ++row)
    {
        word charWord = ((word) charBitmask[row]) << 8;  // load into A, reset B; high nybble of D now init
        byte numShifts = frameBitOffsetTable[frameCol];
        charWord = charWord >> numShifts;

        word d = *screenWord;  // read screen bits (big endian word read)
        word mask = frameMaskTable[frameCol];
        if (asciiCode)
        {
            d &= mask;
            d |= charWord;
        }
        else
        {
            d = (d & mask) | (d ^ ~mask);  // invert colors
        }
        *screenWord = d;

        screenWord += 16;  // point to next row (32 bytes down)
    }
#else  // Equivalent code in assembler:
    byte frameCol;
    word *screenWord;
    byte *charBitmask;
    word numShifts;
    word mask;
    word invMask;
    word charWord;
    word temp;
    word leftTerm;
    word d;
    byte row;

    asm
    {
        ldb     :x      // Colon before name forces ref to C variable named 'x', not register X.
        andb    #7
        stb     frameCol

        //word *screenWord = (word *) (textScreenBuffer + y * 256 + x / 8 * 5 + frameByteAddrTable[frameCol]);
        ldb     frameCol
        leax    frameByteAddrTable
        ldb     b,x
        stb     row                     // row used as temp storage

        ldb     :x
        lsrb
        lsrb
        lsrb
        lda     #5
        mul

        addb    row
        adca    #0

        adda    :y
        addd    textScreenBuffer
        std     screenWord

        //byte *charBitmask = font4x8 + (((word) asciiCode - (asciiCode < 128 ? 32 : 64)) << 3);
        ldb     asciiCode
        bpl     writeCharAt_sub32
        subb    #64             // assuming B in 160..255
        bra     writeCharAt_sub_done
writeCharAt_sub32:
        subb    #32
writeCharAt_sub_done:
        clra
        lslb
        rola
        lslb
        rola
        lslb
        rola
        leax    font4x8
        leax    d,x
        stx     charBitmask

        //word numShifts = frameBitOffsetTable[frameCol];
        ldb     frameCol
        leax    frameBitOffsetTable
        ldb     b,x
        clra
        std     numShifts

        //word mask = frameMaskTable[frameCol];
        ldb     frameCol
        lslb                    // index in array of words
        leax    frameMaskTable
        ldd     b,x
        std     mask

        //word invMask = ~mask;
        coma
        comb
        std     invMask

        clr     row

writeCharAt_for:
        //charWord = ((word) charBitmask[row]) << 8;  // load into A, reset B; high nybble of D now init
        ldx     charBitmask
        ldb     row
        lda     b,x
        clrb

        //charWord = charWord >> numShifts;
        ldx     numShifts       // counter
        beq     writeCharAt_shifts_done
writeCharAt_shift_loop:
        lsra
        rorb
        leax    -1,x
        bne     writeCharAt_shift_loop
writeCharAt_shifts_done:
        std     charWord

        //word d = *screenWord;  // read screen bits (big endian word read)
        ldx     screenWord
        ldd     ,x
        std     :d
        //if (asciiCode)
        tst     asciiCode
        beq     writeCharAt_nul

        //d &= mask;
        //d |= charWord;
        ldd     :d
        anda    mask
        andb    mask[1]         // non-standard notation: adds one to offset applied on U
        ora     charWord
        orb     charWord[1]
        std     :d
        bra     writeCharAt_after_if

        //else: d = (d & mask) | (d ^ invMask);  // invert colors
writeCharAt_nul:
        std     temp
        anda    mask
        andb    mask[1]
        std     leftTerm

        ldd     temp
        eora    invMask
        eorb    invMask[1]

        ora     leftTerm
        orb     leftTerm[1]
        std     :d
writeCharAt_after_if:

        //*screenWord = d;
        //screenWord += 16;  // point to next row (32 bytes down)
        ldx     screenWord
        ldd     :d
        std     ,x
        leax    BYTES_PER_PIXEL_ROW,x
        stx     screenWord

        ldb     row
        incb
        stb     row
        cmpb    #PIXEL_ROWS_PER_TEXT_ROW
        blo     writeCharAt_for
    }
#endif
}


void invertPixelsAtCursor()
{
    byte x = textPosX;
    byte y = textPosY;

    if (x >= HIRESWIDTH)  // logical position that gets mapped to start of next line
    {
        x = 0;
        ++y;
        if (y >= HIRESHEIGHT)
            y = HIRESHEIGHT - 1;
    }

    writeCharAt(x, y, 0);
}


void scrollTextScreenUp()
{
    word *end = (word *) (textScreenBuffer
                            + (word) BYTES_PER_PIXEL_ROW
                                     * (PIXEL_ROWS_PER_SCREEN
                                        - PIXEL_ROWS_PER_TEXT_ROW));
                // start of last row: 32 bytes * (192 rows - 8)
    asm
    {
        ldx     textScreenBuffer
@scrollTextScreen_loop1:
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
        ldd     BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW,x
        std     ,x++
;
        cmpx    end
        blo     @scrollTextScreen_loop1

        tfr     x,d
        addd    #BYTES_PER_PIXEL_ROW*PIXEL_ROWS_PER_TEXT_ROW
        std     end
        ldd     #$FFFF
@scrollTextScreen_loop2:
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        std     ,x++
        cmpx    end
        blo     @scrollTextScreen_loop2
    }
}


void moveCursor(byte x, byte y)
{
    if (x >= HIRESWIDTH)
        return;
    if (y >= HIRESHEIGHT)
        return;

    textPosX = x;
    textPosY = y;
}


void goToNextRowIfPastEndOfRow()
{
    if (textPosX < HIRESWIDTH)  // if not past end of current row
        return;

    textPosX = 0;
    ++textPosY;
    if (textPosY < HIRESHEIGHT)  // if not past bottom of screen
        return;

    scrollTextScreenUp();
    textPosY = HIRESHEIGHT - 1;
}


void writeChar(byte ch)
{
    if (ch == '\a')
        sound(1, 1);
    else if (ch == '\b')
    {
        // Non-destructive backspace. At (0, 0), does not scroll screen up.
        //
        if (textPosX > 0)
            --textPosX;
        else if (textPosY > 0)
        {
            textPosX = HIRESWIDTH - 1;
            --textPosY;
        }
    }
    else if (ch == '\t')
    {
        // If past end of current row, start by putting cursor at start of next row.
        // Scroll screen up one row if needed.
        // Then tab processing can start at column 0.
        //
        goToNextRowIfPastEndOfRow();

        byte newX = (textPosX | 7) + 1;
        if (newX > HIRESWIDTH)  // tab at 48..50 leads to 56: clamp
            newX = HIRESWIDTH;
        for (byte numSpaces = newX - textPosX; numSpaces; --numSpaces)
            writeString(" ");
    }
    else if (ch == '\n')
    {
        // Note that textPosX may be HIRESWIDTH at this point.
        //
        textPosX = 0;
        ++textPosY;
        if (textPosY >= HIRESHEIGHT)
        {
            scrollTextScreenUp();
            textPosY = HIRESHEIGHT - 1;
        }
    }
    else if (ch == '\f')
    {
        clrscr();
    }
    else if (ch == '\r')
    {
        textPosX = 0;
    }
    else if (ch >= ' ' && ch != 127)  // printable character:
    {
        // If past end of current row, put cursor at start of next row.
        // Scroll screen up one row if needed.
        //
        goToNextRowIfPastEndOfRow();

        // Write char at cursor, advance cursor, go to next line if needed.
        //
        writeCharAt(textPosX, textPosY, ch);
        ++textPosX;

        // textPosX may now be HIRESHEIGHT, which is not a writable column,
        // but we tolerate it for easier management of a newline that comes
        // after writing 51 chars on a row.
    }
}


void writeString(char *str)
{
    for (;;)
    {
        byte ch = *str;
        if (!ch)
            break;
        ++str;

        writeChar(ch);
    }
}


void writeCenteredLine(byte row, char *line)
{
    moveCursor((byte) ((HIRESWIDTH - strlen(line)) / 2), row);
    writeString(line);
}


void writeDecWord(word w)
{
    char buf[11];
    writeString(dwtoa(buf, 0, w));
}


void clrtoeol()
{
    for (byte x = textPosX; x != HIRESWIDTH; ++x)
        writeCharAt(x, textPosY, ' ');
}


void clrtobot()
{
    clrtoeol();
    clearRowsToEOS(0xff, textPosY + 1);
}


void removeCursor()
{
    if (hiResTextCursorPresent)
    {
        invertPixelsAtCursor();
        hiResTextCursorPresent = 0;
    }
}


void animateCursor()
{
    // Look at bits 4 and 5 of TIMER.
    // Values 0..2 mean displayed cursor, 3 means no cursor.
    // Resetting TIMER at 0 will display the cursor.
    //
    byte currentCursorState = (getTimer() & 0x30) != 0x30;
    if (currentCursorState != hiResTextCursorPresent)
    {
        invertPixelsAtCursor();
        hiResTextCursorPresent ^= 1;
    }
}


byte waitKeyBlinkingCursor()
{
    byte key;
    while (!(key = inkey()))
        animateCursor();
    removeCursor();
    return key;
}


///////////////////////////////////////////////////////////////////////////////


#ifndef HIRESTEXT_NO_VT52


// VT52 state machine states.
//
enum
{
    VT52_TEXT           = 0,  // Chars interpreted as text to display, except Escape.
    VT52_GOT_ESC        = 1,  // Escape has just been received.
    VT52_WANT_LINE      = 2,  // Y command waiting for line character.
    VT52_WANT_COL       = 3,  // Y command waiting for column character.
    VT52_IGNORE_NEXT    = 4,  // Ignore next byte(s), depending on 'vt52NumBytesToIgnore'.
};


byte vt52State = VT52_TEXT;
byte vt52Line = 0;  // 0..HIRESHEIGHT-1, unlike VT52 spec which starts at 1.
byte vt52NumBytesToIgnore = 0;  // Number of coming bytes that will be ignored (see VT52_IGNORE_NEXT).


void initVT52()
{
    vt52State = VT52_TEXT;
    vt52Line = 0;
}


#endif  /* HIRESTEXT_NO_VT52 */


// Calls writeChar() and moveCursor() to execute the VT52 sequence
// provided by the calls to this function.
//
// Source: http://bitsavers.trailing-edge.com/pdf/dec/terminal/gigi/EK-0GIGI-RC-001_GIGI_Programming_Reference_Card_Sep80.pdf
//
void processConsoleOutChar(byte ch)
{
#ifndef HIRESTEXT_NO_VT52
    if (vt52State == VT52_IGNORE_NEXT)
    {
        --vt52NumBytesToIgnore;
        if (!vt52NumBytesToIgnore)
            vt52State = VT52_TEXT;
        return;
    }
#endif

    removeCursor();

#ifndef HIRESTEXT_NO_VT52
    if (vt52State == VT52_TEXT)
    {
        if (ch == 27)  // if Escape char
            vt52State = VT52_GOT_ESC;
        else
#endif
            writeChar(ch);
        return;
#ifndef HIRESTEXT_NO_VT52
    }

    if (vt52State == VT52_GOT_ESC)
    {
        // Most common commands should be tested first.
        //
        if (ch == 'Y')  // direct cursor address (expecting 2 more bytes)
        {
            vt52State = VT52_WANT_LINE;
            return;
        }
        if (ch == 'K')  // erase to end of line
        {
            clrtoeol();
            vt52State = VT52_TEXT;
            return;
        }
        if (ch == 'D')  // cursor left
        {
            if (textPosX)
                --textPosX;
            vt52State = VT52_TEXT;  // end of sequence
            return;
        }
        if (ch == 'H')  // cursor home
        {
            home();
            vt52State = VT52_TEXT;
            return;
        }
        if (ch == 'J')  // erase to end of screen
        {
            clrtobot();
            vt52State = VT52_TEXT;
            return;
        }
        if (ch == 'A')  // cursor up
        {
            if (textPosY)
                --textPosY;
            vt52State = VT52_TEXT;  // end of sequence
            return;
        }
        if (ch == 'B')  // cursor up
        {
            if (textPosY < HIRESHEIGHT - 1)
                ++textPosY;
            vt52State = VT52_TEXT;  // end of sequence
            return;
        }
        if (ch == 'C')  // cursor right
        {
            if (textPosX < HIRESWIDTH - 1)
                ++textPosX;
            vt52State = VT52_TEXT;  // end of sequence
            return;
        }
        if (ch == 'S')  // mysterious sequence: ESC S O (used by vi...)
        {
            vt52State = VT52_IGNORE_NEXT;
            vt52NumBytesToIgnore = 1;
            return;
        }
        if (ch == 'G')  // select ASCII char set
        {
            vt52State = VT52_TEXT;
            return;
        }

        // Any other sequence is not supported. Return to text mode.
        // G (select ASCII char set) is not supported.
        //
        //writeString("\n\n*** INVALID VT52 COMMAND\n");
        vt52State = VT52_TEXT;
        return;
    }

    if (vt52State == VT52_WANT_LINE)
    {
        vt52Line = ch - 32;
        vt52State = VT52_WANT_COL;
        return;
    }

    if (vt52State == VT52_WANT_COL)
        moveCursor(ch - 32, vt52Line);

    vt52State = VT52_TEXT;
#endif  /* HIRESTEXT_NO_VT52 */
}
