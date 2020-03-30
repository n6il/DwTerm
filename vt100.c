#include "vt100.h"
#ifdef _CMOC_VERSION_
#include <coco.h>
#include <cmoc.h>
#include "dwterm.h"
#include "coco3.h"
#include "coco2.h"
#else
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#endif

#define RETURN_OK 0

#define COLS 80
#define ROWS 24

#define HRESSCRN 0x2000
#define defScreenAddr 0x8000
#define defScreenEnd (screenAddr - 1 + (160*ROWS))
#define scrDiff 0x6000
#define H_CRSLOC 0xfe00
#define   H_CRSX 0xfe02
#define   H_CRSY 0xfe03
#define MAP_IN_SCREEN (*(uint8_t *)0xffa4 = 0x36)
#define MAP_OUT_SCREEN (*(uint8_t *)0xffa4 = 0x3c)



#ifndef NULL
#define NULL 0
#endif

uint8_t vt100buf[16] = {};
uint8_t *vt100bufp = vt100buf;
uint8_t vt100bufi = 0;
uint8_t vt100nums[10];
int8_t vt100numi;
uint8_t *vt100nump = 0; // NULL;

uint8_t savedCol, savedRow;
uint8_t currCol, currRow;
uint8_t *currAddr = 0x8000;
uint8_t currAttr = ATTR_DEFAULT;
uint8_t savedAttr = 0;
uint8_t currAttrUline = 0;
uint8_t currAttrBlink = 0;
uint8_t currAttrReverse = 0;
uint8_t defAttrFg = ATTR_WHITE;
uint8_t defAttrBg = ATTR_BLACK;
uint8_t defAttr;
uint8_t currAttrFg = ATTR_WHITE;
uint8_t currAttrBg = ATTR_BLACK;

uint8_t display_cols = COLS;
uint8_t display_rows = ROWS;
uint8_t hasAttr = 0;
uint8_t bytesCol = 2;
uint8_t bytesRow = 2*COLS;
uint8_t *screenAddr = defScreenAddr;
uint8_t *screenEnd = defScreenEnd;

char prevCh;

enum vt100_state_e {
	STATE_START,
	STATE_ESC,
	STATE_CSI,
	STATE_NUM,
	STATE_SEMI,
	STATE_FINISH,
	STATE_ERROR
};

enum vt100_state_e vt100state = -1;

#ifdef _CMOC_VERSION_
int8_t isgraph(char c)
{
    return (c>=' ' && c<=0x127);
}
#endif

void printline(char *data, int N)
{
	uint8_t i;

	for(i=0;i<N;i++)
		printf("%02x ", data[i]);
	printf(" ");
	for(i=0;i<N;i++)
		if(isgraph((unsigned char) data[i]))
			printf("%c", data[i]);
		else
			printf(".");
	printf("\n");
}


void dw_puts(char *s)
{
#ifdef _CMOC_VERSION_
    while (*s)
        writeChannel(1, *s++);
#else
    printf("Sending back to client: ");
    printline(s, strlen(s));
#endif
}

#ifdef _CMOC_VERSION_
int isprint(char c) {return 0;};
#endif

/*
void STATE_CHANGE(char c, int ns) {
    // if (ns == STATE_FINISH || ns == STATE_ERROR )
        // printf("%02x(%c): state change %d -> %d\n", c, isprint(c)? c:'.', vt100state, ns);
    vt100state = ns;
}
*/
#define STATE_CHANGE(c, ns) vt100state = ns

#ifdef _CMOC_VERSION_
int isdigit(unsigned char c)
{
    return ((c-48) < 10);
}
#endif


uint8_t *get_pos_address(uint8_t col, uint8_t row)
{
    uint16_t a;
    /*
    col -= 1;
    row -= 1;
    a = screenAddr;
    a +=( ((uint16_t)2 * display_cols) * row);
    a += (col*2);
    */
    a = screenAddr;
    asm 
    {
        * a += display_cols * (row-1) * 2
        lda :display_cols
        ldb :row
        decb
        mul
        tst :hasAttr
        beq cont@
        aslb
        rola
cont@
        addd :a
        std :a

        * a += (col-1) * 2
        clra
        ldb :col
        decb
        tst :hasAttr
        beq cont@
        aslb
        rola
cont@
        addd :a
        std :a
    }
    return (uint8_t *)a;
}

void mapMmu(uint8_t task, uint8_t bank, uint8_t block)
{
    *((uint8_t *)0xffa0 + (task*8) + bank) = block;
}


void move_cursor(uint8_t col, uint8_t row) {
    uint8_t *newAddr;

    // wrapCheck(col, row);
    asm
    {
        clra

        ldb :col
        bne colhi@
        incb
        stb :col
colhi@:
        cmpb :display_cols
        bls coldone@
        ldb :display_cols
        stb :col
coldone@:
        stb :currCol

        ldb :row
        bne rowhi@
        incb
        stb :row
rowhi@
        cmpb :display_rows
        bls rowdone@
        ldb :display_rows
        stb :row
rowdone@
        stb :currRow
    }

    newAddr = get_pos_address(col, row);

    // doMoveCursor(newAddr);
    disableInterrupts();
    if (hasAttr) {
        MAP_IN_SCREEN; // mapMmu(0, 4, 0x36); // Map in text screen block 6.6 at 0x8000
    }
    
    asm {
        tst :hasAttr
        beq noAttr
        * *(currAddr + 1) = savedAttr; // currAttr;
        ldx :currAddr
        ldb :savedAttr
        stb 1,x

        * savedAttr = *(newAddr+1);
        ldx :newAddr
        ldb 1,x
        stb :savedAttr

        * *(newAddr+1) ^= ATTR_ULINE;
        eorb #$40
        stb 1,x
        bra storeNewAddr

noAttr:
        ldx :currAddr
        ldb :savedAttr
        stb ,x

        ldx :newAddr
        ldb ,x
        stb :savedAttr

        ldb #$af
        stb ,x

        * currAddr = newAddr;
storeNewAddr:
        stx :currAddr
    }

    if (hasAttr) {
        MAP_OUT_SCREEN; // mapMmu(0, 4, 0x3c); // Restore basic block 7.4 at 0x8000
    }
    enableInterrupts();

    // currCol = col;
    // currRow = row;
    // currAddr = newAddr;
}

void move_window(int n) {
}

void get_curr_pos() {
    currRow = 1 + (*(uint8_t *)H_CRSX);
    currCol = 1 + (*(uint8_t *)H_CRSY);
}


void move_cursor_relative(uint8_t col, uint8_t row) {
    move_cursor(currCol + col, currRow + row);
}

void doScroll()
{
    uint16_t *src = (uint16_t *)(screenAddr + bytesRow);
    uint16_t *dst = (uint16_t *)screenAddr;
    uint16_t *end = (uint16_t *)screenEnd;

    disableInterrupts();
    if (hasAttr) {
        MAP_IN_SCREEN; // mapMmu(0, 4, 0x36); // Map in text screen block 6.6 at 0x8000
        *(currAddr+1) = savedAttr;
    } else {
        *(currAddr) = savedAttr;
    }

    asm
    {
        * copy
        ldx :src
        ldy :dst
        bra @check
@loop:
        ldd ,x++
        std ,y++
@check:
        cmpx :end
        blt @loop

        * clear line
        * ldd #$2000
        tst :hasAttr
        beq @noAttr
        lda #$20
        ldb :defAttr
        bra @check
@noAttr
        ldd #$6060
        bra @check
@loop:
        std ,y++ 
@check:
        cmpy :end
        blt @loop
    }
    if (hasAttr) {
        savedAttr =  *(currAddr+1);
        *(currAddr+1) ^= ATTR_ULINE;
        MAP_OUT_SCREEN; // mapMmu(0, 4, 0x3c); // Restore basic block 7.4 at 0x8000
    } else {
        savedAttr =  *(currAddr);
    }
    enableInterrupts();
}

void vt100_putchar_a()
{
    char ch;
    uint8_t print = 0;
    uint8_t doLf = 0;

    asm
    {
        pshs    x,b  // preserve registers used by this routine
        sta     :ch
    }

    switch(ch)
    {
        case 0x08:
            move_cursor_relative(-1, 0);
            break;

        case 0x09:
            move_cursor((currCol+8)%8, currRow);
            break;

        case 0x0a:
            doLf = 1;
            break;

        case 0x0d:
            move_cursor(1, currRow);
            // doLf = 1;
            break;
        
        default:
            print = 1;
    }

    if (doLf)
    {
        if (currRow == display_rows)
            doScroll();
        else
            move_cursor_relative(0, 1);
    }
    if (print)
    {
        prevCh = ch;
        disableInterrupts();
        if (hasAttr) {
            MAP_IN_SCREEN; // mapMmu(0, 4, 0x36); // Map in text screen block 6.6 at 0x8000
            // *(currAddr+1) = currAttr;
            // savedAttr = currAttr;
            // *(currAddr) = ch;
            asm {
                ldx :currAddr
                lda :currAttr
                sta 1,x
                sta :savedAttr
                ldb :ch
                stb ,x
            }
        } else {
            ch = xlate6847[ch];
            savedAttr = ch;
            *(currAddr) = ch;
        }
        if (hasAttr)
            MAP_OUT_SCREEN; // mapMmu(0, 4, 0x3c); // Restore basic block 7.4 at 0x8000
        enableInterrupts();

        if (currCol == display_cols) {
            if (currRow == display_rows) {
                doScroll();
                move_cursor(1,currRow);
            } else {
                move_cursor(1,currRow+1);
            }
        } else {
            move_cursor_relative(1,0);
        }
    }

    asm
    {
        puls    b,x
    }
}

void vt100_putchar(char ch)
{
    asm
    {
        lda :ch
        lbsr _vt100_putchar_a
    }
}

void vt100_puts(char *s)
{
    while(*s)
        vt100_putchar(*s++);
}

void vt100_putstr(char *s, size_t n)
{
    for(; n>0; n--)
        vt100_putchar(*s++);
}

void erase_block(uint8_t *start, uint8_t *end) 
{
    uint8_t c;
    uint8_t *p;

    disableInterrupts();
    if (hasAttr) {
        MAP_IN_SCREEN; // mapMmu(0, 4, 0x36); // Map in text screen block 6.6 at 0x8000
        c = 0x20;
    } else {
        c = 0x60;
    }
    for (p=start; p<=end; )
    {
            *p++ = c;
            if (hasAttr)
                *p++ = currAttr;
    }
    if (hasAttr)
        MAP_OUT_SCREEN; // mapMmu(0, 4, 0x3c); // Restore basic block 7.4 at 0x8000
    enableInterrupts();
}


void erase_from_here()
{
    // printf ("ED: Erase Display: From here (%d, %d)\n", currRow, currCol);
    erase_block(currAddr, screenEnd);
}
void erase_to_here()
{
    // printf ("ED: Erase Display: To here (%d, %d)\n", currRow, currCol);
    erase_block(screenAddr, currAddr);
}

void clear_screen()
{
    // printf ("ED: Erase Display: Clear Screen\n");
    erase_block(screenAddr, screenEnd);
}

void erase_line_from_here()
{
    // printf ("ED: Erase Display: Line From here (%d, %d)\n", currRow, currCol);
    erase_block(currAddr, get_pos_address(display_cols-1, currRow));
}

void erase_line_to_here()
{
    // printf ("ED: Erase Display: Line To here (%d, %d)\n", currRow, currCol);
    erase_block(get_pos_address(0, currRow), currAddr);
}

void clear_line()
{
    // printf ("ED: Erase Display: Clear Line: %d\n", currRow);
    erase_block(get_pos_address(0, currRow), get_pos_address(display_cols-1, currRow));
}

void delete_chars_in_line(uint8_t n)
{
    uint8_t *dst = currAddr;
    uint8_t *src = get_pos_address(currCol + n, currRow);
    uint8_t count = display_cols - currCol - n;
    uint8_t c;


    if (hasAttr) {
        count *= bytesCol;
        c = 0x20;
    } else {
        c = 0x60;
    }

    disableInterrupts();
    if  (hasAttr)
        MAP_IN_SCREEN;
    for ( ; count>0; count--)
        *(dst++) = *(src++);

    for ( ; n>0; n--) {
        *(dst++) = c;
        if (hasAttr)
            *(dst++) = defAttr;
    }
    if (hasAttr)
        MAP_OUT_SCREEN;
    enableInterrupts();
}


/*
 * parse VT100 codes
 * Input: char to process
 * Output:
 *      0 - parsing successful, caller should skip processing this character
 *      n - Number of characters stored in the buffer which caller needs to
 *          process.  After processing the buffer caller should vt100_init.
 *          After this caller should process the current character normally.
 * References:
 *      vt100 user guide chapter 3
 */

int vt100getnum(char *p1, char *p2) {
    int n;
    char p2temp = *p2;
    *p2 = 0;
    n = atoi(p1);
    *p2 = p2temp;
    return n;
}

void sgrClear(void)
{
    if (!hasAttr)
        return;
    currAttr = 0;
    currAttr |= defAttrBg;
    currAttr |= (defAttrFg << 3);
    currAttrBg = defAttrBg;
    currAttrFg = defAttrFg;
    currAttrReverse = 0;
    currAttrUline = 0;
    currAttrBlink = 0;
}

void sgrProc(void)
{
    uint8_t i,n;
    if (!hasAttr)
        return;
    for (i=0 ; i<vt100numi; i++) {
        n = vt100nums[i];
        // Underline
        if (n ==  0) {
            sgrClear();
            continue;
        } else if (n ==  4) {
            currAttrUline = ATTR_ULINE;
        } else if (n == 5) {
            currAttrBlink = ATTR_BLINK;
        } else if (n == 7) {
            currAttrReverse = 1;
        } else if (n ==  10) {
            sgrClear();
            continue;
        } else if (n == 24) {
            currAttrUline = 0;
        } else if (n == 25 ) {
            currAttrBlink = 0;
        } else if (n == 27 ) {
            currAttrReverse = 0;
        } else if ( n>=30 && n<38 ) {
            currAttrFg = n - 30;
        } else if ( n == 38 ) {
            currAttrUline = ATTR_ULINE;
            currAttrFg = defAttrFg;
            currAttrBg = defAttrBg;
        } else if ( n == 39 ) {
            currAttrUline = 0;
            currAttrFg = defAttrFg;
            currAttrBg = defAttrBg;
        } else if ( n>=40 && n<48 ) {
            currAttrBg = n - 40;
        } else if ( n == 49 ) {
            currAttrBg = defAttrBg;
        }
    }

    if (currAttrReverse) {
        currAttr &= ATTR_COLOR_MASK;
        currAttr |= currAttrFg;
        currAttr |= (currAttrBg << 3);
    } else {
        currAttr &= ATTR_COLOR_MASK;
        currAttr |= currAttrBg;
        currAttr |= (currAttrFg << 3);
    }

    currAttr &= ATTR_CLR;
    currAttr |= (currAttrUline & ATTR_ULINE);
    currAttr |= (currAttrBlink & ATTR_BLINK);
    savedAttr = currAttr;
}

void vt100_init(void) {
    STATE_CHANGE(0, STATE_START);
    vt100bufp = vt100buf;
    vt100bufi = 0;
    vt100nump = NULL;
    vt100numi = 0;
}

void setupBorder(void)
{
    *(uint8_t *)0xff9a = defAttrBg;
}

void vt100_setup(uint8_t cols, uint8_t rows, uint8_t attrFlag, uint8_t* startAddr) {
    display_cols = cols;
    display_rows = rows;

    hasAttr = attrFlag;
    if (hasAttr) {
        defAttr = (defAttrFg << 3) | defAttrBg;
        setupPalette();
        setupBorder();
        sgrClear();
        savedAttr = currAttr;
        bytesCol = 2;
    } else {
        bytesCol = 1;
    }
    bytesRow = display_cols * bytesCol;

    screenAddr = startAddr;
    currAddr = startAddr;
    screenEnd = startAddr + ((uint16_t)bytesRow * display_rows) - 1;

    clear_screen();
    move_cursor(1,1);
    vt100_init();
    setConsoleOutHook(vt100_putchar_a); 
}

int vt100(char c) {
    uint8_t i, j;
    char *p;

    // If the previous state was STATE_NUM and the incoming character is
    // not numeric, then the number string starting at vt100nump can be
    // read in now.
    if (vt100state == STATE_NUM && !isdigit(c)) {
        *vt100bufp = 0;
        vt100nums[vt100numi++] = (uint8_t)atoi((const char *)vt100nump);
    }

    // Main parser logic
    if ( vt100state == STATE_START ) {
        // Start of an escape sequence
        if ( c == '\x1b' ) {
            if (vt100state == STATE_START) {
                STATE_CHANGE(c, STATE_ESC);
            } else {
                STATE_CHANGE(c, STATE_ERROR);
            }
        } else {
            STATE_CHANGE(c, STATE_ERROR);
        }
    } else {
        switch(c) {
            // Start of an escape sequence
            case '\x1b':
                if (vt100state == STATE_START) {
                    STATE_CHANGE(c, STATE_ESC);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // Start of a CSI sequence
            case '[':
                if (vt100state == STATE_ESC) {
                    STATE_CHANGE(c, STATE_CSI);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // Numeric parameters: must follow ESC, CSI, or ;
            // vt100nump points to the start of the string
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                if (vt100state == STATE_ESC || vt100state == STATE_CSI ||
                        vt100state == STATE_SEMI) {
                    vt100nump = vt100bufp;
                    STATE_CHANGE(c, STATE_NUM);
                } else if (vt100state == STATE_NUM) {
                    STATE_CHANGE(c, STATE_NUM);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // Semicolons delimit numeric parameters. Valid if a semicolon
            // immediately follows a number
            case ';':
                if (vt100state == STATE_NUM) {
                    STATE_CHANGE(c, STATE_SEMI);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // CUU Cursor Up
            case 'A':
                if (vt100state == STATE_CSI) {
                    i = - 1;
                } else if (vt100state == STATE_NUM) {
                    i = - vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                move_cursor_relative(0, i);
                // printf("CUU: Up %d\n" , i);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // CUD Cursor Down
            case 'B':
                if (vt100state == STATE_CSI) {
                    i = 1;
                } else if (vt100state == STATE_NUM) {
                    i = vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                move_cursor_relative(0, i);
                // printf("CUD: Down %d\n" , i);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // HPR - Character Position Forward
            case 'a':
            // CUF Cursor Forward
            case 'C':
                if (vt100state == STATE_CSI) {
                    j =  1;
                } else if (vt100state == STATE_NUM) {
                    j = vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                // i = j > display_cols ? display_cols : j;
                // printf("CUF: Forward %d" , i);
                move_cursor_relative(j, 0);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // CUB Cursor backward
            case 'D':
                if (vt100state == STATE_CSI) {
                    i = - 1;
                } else if (vt100state == STATE_NUM) {
                    i = - vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                move_cursor_relative(i, 0);
                // printf("CUB: Backward %d\n" , i);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // NEL - Next Line, CNL Cursor Next Line
            case 'E':
                if (vt100state == STATE_ESC || vt100state == STATE_CSI) {
                    j = currRow + 1;
                } else if (vt100state == STATE_NUM) {
                    j = currRow + vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                // i = j > display_rows ? display_rows : j;
                move_cursor(1, j);
                // printf("CNL: Next Line %d\n", i);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // CPL Cursor Preceeding Line
            case 'F':
                if (vt100state == STATE_CSI) {
                    j = currRow - 1;
                } else if (vt100state == STATE_NUM) {
                    j = currRow - vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                i = j < 1 ? 1 : j;
                // printf("CPL: Prev Line %d\n", i);
                move_cursor(1, i);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // CHA Cursor Character Absolute
            case 'G':
                if (vt100state == STATE_CSI) {
                    i = 1;
                } else if (vt100state == STATE_NUM) {
                    i = vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                // printf("CHA: Cursor Character Absolute: %d\n", i);
                move_cursor(i, currRow);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // CUP (Cursor Position)
            case 'f':
            case 'H':
                if (vt100state == STATE_NUM && vt100numi == 2) {
                    // printf("CUP: Cursor Position\n");
                    move_cursor(vt100nums[1], vt100nums[0]);
                    STATE_CHANGE(c, STATE_FINISH);
                } else if (vt100state == STATE_CSI) {
                    // printf("CUP: Cursor Position\n");
                    move_cursor(1, 1);
                    STATE_CHANGE(c, STATE_FINISH);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // ED Erase In Page (Erase Display)
            case 'J':
                if (vt100state == STATE_CSI) {
                    erase_from_here();
                    STATE_CHANGE(c, STATE_FINISH);
                } else if (vt100state == STATE_NUM) {
                    i = vt100nums[0];
                    if (i == 0) {
                        erase_from_here();
                        STATE_CHANGE(c, STATE_FINISH);
                    } else if (i == 1) {
                        erase_to_here();
                        STATE_CHANGE(c, STATE_FINISH);
                    } else if (i == 2 || i == 3 /* Linux */ ) {
                        clear_screen();
                        move_cursor(1,1);
                        STATE_CHANGE(c, STATE_FINISH);
                    } else {
                        STATE_CHANGE(c, STATE_ERROR);
                    }
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // EL Erase Line (ANSI.SYS)
            case 'K':
                if (vt100state == STATE_CSI) {
                    erase_line_from_here();
                    STATE_CHANGE(c, STATE_FINISH);
                } else if (vt100state == STATE_NUM) {
                    i = vt100nums[0];
                    if (i == 0) {
                        erase_line_from_here();
                        STATE_CHANGE(c, STATE_FINISH);
                    } else if (i == 1) {
                        erase_line_to_here();
                        STATE_CHANGE(c, STATE_FINISH);
                    } else if (i == 2 || i == 3 /* Linux */ ) {
                        clear_line();
                        STATE_CHANGE(c, STATE_FINISH);
                    } else {
                        STATE_CHANGE(c, STATE_ERROR);
                    }
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // DCH - Delete Character
            case 'P':
                if (vt100state == STATE_CSI) {
                    i =  1;
                } else if (vt100state == STATE_NUM) {
                    i =  vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                delete_chars_in_line(i);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // SEE - Select Editing Extent
            case 'Q':
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // DA (Device Attributes) (VT100)
            // Must follow a CSI or a number (number is ignored)
            case 'c':
                if (vt100state == STATE_CSI || vt100state == STATE_NUM ) {
                    // 0 - no options 2 advanced video
                    vt100numi = 0; 
                    sprintf(vt100buf, "\x1b[?1;%dc", vt100numi);
                    dw_puts(vt100buf);
                    STATE_CHANGE(c, STATE_FINISH);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                } 
                break;

            // REP - Repeat
            case 'b':
                if (vt100state == STATE_CSI) {
                    i =  1;
                } else if (vt100state == STATE_NUM) {
                    i =  vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                for ( ; i>0; i--)
                    vt100_putchar(prevCh);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // HPA - CHARACTER POSITION ABSOLUTE
            case 'd':
                if (vt100state == STATE_CSI) {
                    i =  1;
                } else if (vt100state == STATE_NUM) {
                    i =  vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                move_cursor(currCol, i);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            // SGR (Set Graphic Rendition
            case 'm':
                if (vt100state == STATE_NUM) {
                    // for (i=0; i<vt100numi; i++)
                        // printf("SGR: Set attribute: %d\n", vt100nums[i]);
                    sgrProc();
                    STATE_CHANGE(c, STATE_FINISH);
                } else if (vt100state == STATE_CSI) {
                    // printf("SGR: Set attribute: %d\n", 0);
                    sgrClear();
                    STATE_CHANGE(c, STATE_FINISH);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            case 'n':
                if (vt100state == STATE_NUM ) {
                    // DSR (Device Status Report)
                    if (vt100nums[0] == 5) {
                        strcpy(vt100buf, "\x1b[0n");
                    } else if (vt100nums[0] == 6) {
                        // CPR -- Current Position Report
                        sprintf(vt100buf, "\x1b[%d;%dR", currRow, currCol);
                    } else {
                        STATE_CHANGE(c, STATE_ERROR);
                        break;
                    }

                    dw_puts(vt100buf);
                    goto vt100finish;
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // Save Cursor Position (ANSI.SYS)
            case 's':
                if (vt100state == STATE_CSI) {
                    savedCol = currCol;
                    savedRow = currRow;
                    STATE_CHANGE(c, STATE_FINISH);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // Restore Cursor Position (ANSI.SYS)
            case 'u':
                if (vt100state == STATE_CSI) {
                    move_cursor(savedCol, savedRow);
                    STATE_CHANGE(c, STATE_FINISH);
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                }
                break;

            // HPA Character Position Absolute
            case '`':
                if (vt100state == STATE_CSI) {
                    i =  1;
                } else if (vt100state == STATE_NUM) {
                    i =  vt100nums[0];
                } else {
                    STATE_CHANGE(c, STATE_ERROR);
                    break;
                }

                move_cursor(i, currRow);
                STATE_CHANGE(c, STATE_FINISH);
                break;

            default:
                STATE_CHANGE(c, STATE_ERROR);
        };
    }

    if (vt100state == STATE_FINISH) {
        vt100_init();
        return RETURN_OK;
    } else if (vt100state == STATE_ERROR) {
        STATE_CHANGE(c, STATE_START);
        (*vt100bufp++) = c;
        vt100bufi++;
        // i = (uint8_t)(vt100bufp - vt100buf);
        // printf("Print %d chars\n", i);
        // printline(vt100buf, i);
        if (vt100bufi == 1) {
            asm {
                lda :c
                lbsr vt100_putchar_a
            }
        } else {
            vt100_putstr((const char *)vt100buf, vt100bufi);
        }
        vt100_init();
        return (vt100bufp - vt100buf);
    } 

    // Save the character in the buffer
    (*vt100bufp++) = c;
    return RETURN_OK;

vt100error:
    STATE_CHANGE(c, STATE_START);
    return (vt100bufp - vt100buf);

vt100finish:
    vt100_init();
    return RETURN_OK;
}


/*
# vim: ts=4 sw=4 sts=4 expandtab
*/
