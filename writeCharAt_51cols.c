/*  writeCharAt_51cols.c

    By Pierre Sarrazin <http://sarrazip.com/>
    This file is in the public domain.
*/

#include "hirestxt.h"

#include "font4x8.h"


// About the 51x24 mode:
//
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


// frameByteAddrTable_51cols[c] is the byte offset in a frame of
// the c-th (0..7) character in that frame.
// frameBitOffsetTable_51cols[c] is the number of right shifts to apply
// to position bits at the c-th (0..7) character of a frame.
// frameMaskTable_51cols[c] is the AND mask that applies at the c-th (0..7)
// character of a frame.
//
static const byte frameByteAddrTable_51cols[8]  = { 0, 0, 1, 1, 2, 3, 3, 4 };
static const byte frameBitOffsetTable_51cols[8] = { 0, 5, 2, 7, 4, 1, 6, 3 };
static const word frameMaskTable_51cols[8] = { 0x07ff, 0xf83f, 0xc1ff, 0xfe0f, 0xf07f, 0x83ff, 0xfc1f, 0xe0ff };


void putBitmaskInScreenWord(byte asciiCode, word *screenWord,
                            const byte *charBitmask, word charWordShifts,
                            word mask)
{
    word charWord, d, invMask, temp, leftTerm, row;
    asm
    {
; Cache an inverted mask.
        ldd     :mask
        coma
        comb
        std     :invMask

; Initialize for() loop counter.
        clrb                ; B caches :row
        stb     :row        ; stb is 2 cycles less than clr

@writeCharAt_for
; charWord = ((word) charBitmask[row]) << 8;
        ldx     :charBitmask
        lda     b,x             ; B caches :row
        clrb                    ; D is now charWord
;
; charWord >>= charWordShifts;
        ldx     :charWordShifts       ; counter
        beq     @writeCharAt_shifts_done
@writeCharAt_shift_loop               ; shift D right X times
        lsra
        rorb
        leax    -1,x
        bne     @writeCharAt_shift_loop
@writeCharAt_shifts_done
        std     :charWord
;
; word d = *screenWord;  ; read screen bits (big endian word read)
        ldx     :screenWord
        ldd     ,x
        std     :d
; if (asciiCode)
        tst     :asciiCode
        beq     @writeCharAt_nul
;
; d &= mask;
; d |= charWord;
        anda    :mask
        andb    :mask[1]         ; adds one to offset applied on U
        ora     :charWord
        orb     :charWord[1]
        std     :d
        bra     @writeCharAt_after_if
;
; else: d = (d & mask) | (d ^ invMask);
@writeCharAt_nul
        std     :temp
        anda    :mask
        andb    :mask[1]
        std     :leftTerm
;
        ldd     :temp
        eora    :invMask
        eorb    :invMask[1]
;
        ora     :leftTerm
        orb     :leftTerm[1]
        std     :d
;
@writeCharAt_after_if
;
; *screenWord = d;
; screenWord += 16;
        std     [:screenWord]
        leax    BYTES_PER_PIXEL_ROW,x   ; N.B.: X still contains :screenWord, so increment it
        stx     :screenWord
;
        ldb     :row
        incb
        stb     :row
        cmpb    #PIXEL_ROWS_PER_TEXT_ROW
        blo     @writeCharAt_for
    }
}


void writeCharAt_51cols(byte x, byte y, byte asciiCode)
{
#if 0 // Original (tested) code in C:
    const byte frameCol = x % 8;
    word *screenWord = (word *) (textScreenBuffer + ((word) y * 256) + x / 8 * 5 + frameByteAddrTable_51cols[frameCol]);
        // 256 = 8 rows per character, times 32 bytes per pixel row
        // 8 = 8 chars par frame. 5 = 5 bytes per frame.

    const byte charWordShifts = frameBitOffsetTable_51cols[frameCol];
    const word mask = frameMaskTable_51cols[frameCol];

    // In charBitmask, only the high 5 bits of each byte are significant.
    // The others must be 0.
    //
    const byte *charBitmask = font4x8 + (((word) asciiCode - (asciiCode < 128 ? 32 : 64)) << 3);

    for (byte row = 0; row < PIXEL_ROWS_PER_TEXT_ROW; ++row)
    {
        word charWord = ((word) charBitmask[row]) << 8;  // load into A, reset B; high nybble of D now init
        charWord >>= charWordShifts;

        word d = *screenWord;  // read screen bits (big endian word read)
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
    word charWordShifts;

    asm
    {
        ldb     :x
        andb    #7                      ; % 8
        stb     :frameCol

; word *screenWord = (word *) (textScreenBuffer + ((word) y * 256) + x / 8 * 5 + frameByteAddrTable_51cols[frameCol]);
        ldb     :frameCol
        leax    :frameByteAddrTable_51cols
        ldb     b,x
        pshs    b

        ldb     :x
        lsrb
        lsrb
        lsrb                            ; x / 8
        lda     #5
        mul

        addb    ,s+
        adca    #0

        adda    :y                      ; add y * 256 to D
        addd    :textScreenBuffer
        std     :screenWord

; byte *charBitmask = font4x8 + (((word) asciiCode - (asciiCode < 128 ? 32 : 64)) << 3);
        ldb     :asciiCode
        bpl     @writeCharAt_sub32      ; if 0..127
        subb    #64                     ; assuming B in 160..255
        bra     @writeCharAt_sub_done
@writeCharAt_sub32
        subb    #32
@writeCharAt_sub_done
        clra                            ; D = result of subtraction; shift this 3 bits left
        lslb
        rola
        lslb
        rola
        lslb
        rola
        leax    :font4x8
        leax    d,x
        stx     :charBitmask

; word charWordShifts = frameBitOffsetTable_51cols[frameCol];
        ldb     :frameCol
        leax    :frameBitOffsetTable_51cols
        ldb     b,x
        clra
        std     :charWordShifts

; word mask = frameMaskTable_51cols[frameCol];
        ldb     :frameCol
        lslb                            ; index in array of words
        leax    :frameMaskTable_51cols
        ldd     b,x

; Call putBitmaskInScreenWord(byte asciiCode, word *screenWord, const byte *charBitmask, word charWordShifts, word mask)
;
        pshs    b,a
        ldx     :charWordShifts
        ldd     :charBitmask
        pshs    x,b,a
        ldx     :screenWord
        ldb     :asciiCode
        pshs    x,b,a                   ; A is garbage: does not matter
        lbsr    putBitmaskInScreenWord
        leas    10,s
    }
#endif
}
