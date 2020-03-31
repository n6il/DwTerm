*******************************************************
*
* DWInit
*    Initialize DriveWire for CoCo Bit Banger

DWInit
               IFNE     ARDUINO

* setup PIA PORTA (read)
               clr       $FF51
               clr       $FF50
               lda       #$2C
               sta       $FF51

* setup PIA PORTB (write)
               clr       $FF53
               lda       #$FF
               sta       $FF52
               lda       #$2C
               sta       $FF53
               rts

               ELSE

               pshs      a,x

               IFNE COCO3FPGAWIFI
               clr BCKCTRL 115200 baud 
more@
               lda BCKCTRL
               bita #$02
               beq done@
               lda BCKDATA
               bra more@
done@
               ENDC

               IFNE MEGAMINIMPI
		pshs b,cc
		orcc #$50	                clear interrupts

		lda MPIREG               	save mpi settings
		pshs a
		anda #CTSMASK               	Save previous CTS, clear off STS
		ora #MMMSLT                     Set STS for MMMPI Uart Slot
		sta MPIREG

                sta MMMUARTB+RESET               Reset UART

		lda #LCR8BIT|LCRPARN	        LCR: 8N1,DLAB=0
		sta MMMUARTB+LCR

		clr MMMUARTB+IER                  IER: disable interrupts

		lda #FCRFEN|FCRRXFCLR|FCRTXFCLR|FCRTRG8B FCR: enable,clear fifos, 8-byte trigger
		sta MMMUARTB+FCR

                lda #MCRDTREN|MCRRTSEN|MCRAFEEN	 MCR: DTR & Auto Flow Control
		sta MMMUARTB+MCR

		lda MMMUARTB+LCR	                enable DLAB
                ora #DLABEN
                sta MMMUARTB+LCR

		ldd #B921600                    Set Divisor Latch
		; std MMMUARTB+DL16                 16-bit DL helper
		sta MMMUARTB+DLM
		stb MMMUARTB+DLL

		lda MMMUARTB+LCR                  disable DLAB
		anda #DLABDIS
		sta MMMUARTB+LCR

clrloop@	lda MMMUARTB+LSR                  check RX FiFo Status
		bita #LSRDR
		beq restore@                    its empty
        	lda MMMUARTB	                dump any data that's there
		bra clrloop@
restore@

                puls a		                restore mpi settings
		sta MPIREG
		puls b,cc
	       ENDC

               IFDEF     PIA1Base
               ldx       #PIA1Base           $FF20
               clr       1,x                 clear CD
               lda       #%11111110
               sta       ,x
               lda       #%00110100
               sta       1,x
               lda       ,x
               ENDC
               puls       a,x,pc

               ENDC
