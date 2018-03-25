CC = cmoc
CFLAGS = 

OTHEROBJS = hirestxt.o font4x8.o
BINS = DWTRMBCK.BIN DWTRMBB.BIN DWTRMBB1.BIN DWTRM232.BIN DWTRMWI.BIN DWTRMB63.BIN

all: DWTERM.dsk
	

clean:
	rm -f *.o *.s *.lst *.map *.i
	rm -f DWTRM*.BIN
	rm -f DWTERM.DSK

dw_becker.o: drivewire.c dwread.asm dwwrite.asm
	$(CC) $(CFLAGS) -DDW_BECKER=1 -c -o $@ $<

DWTRMBCK.BIN: dwterm.c $(OTHEROBJS) dw_becker.o
	$(CC) $(CFLAGS) -DDW_BECKER=1 -o $@ $^ 

dw_bb.o: drivewire.c dwread.asm dwwrite.asm
	$(CC) $(CFLAGS) -c -o $@ $<

DWTRMBB.BIN: dwterm.c $(OTHEROBJS) dw_bb.o
	$(CC) $(CFLAGS) -o $@ $^

dw_bb6309.o: drivewire.c dwread.asm dwwrite.asm
	$(CC) $(CFLAGS) -DDW_H6309 -c -o $@ $<

DWTRMB63.BIN: dwterm.c $(OTHEROBJS) dw_bb.o
	$(CC) $(CFLAGS) -DDW_H6309 -o $@ $^

dw_bb1.o: drivewire.c dwread.asm dwwrite.asm
	$(CC) $(CFLAGS) -DDW_BAUD38400=1 -c -o $@ $<

DWTRMBB1.BIN: dwterm.c $(OTHEROBJS) dw_bb1.o
	$(CC) $(CFLAGS) -DDW_BAUD38400=1 -o $@ $^

dw_rs232pak.o: drivewire.c dwread.asm dwwrite.asm
	$(CC) $(CFLAGS) -DDW_SY6551N=1 -c -o $@ $<

DWTRM232.BIN: dwterm.c $(OTHEROBJS) dw_rs232pak.o
	$(CC) $(CFLAGS) -DDW_SY6551N=1 -o $@ $^

dw_coco3fpgawifi.o: drivewire.c dwread.asm dwwrite.asm
	$(CC) $(CFLAGS) -DDW_COCO3FPGAWIFI=1 -DDW_BECKER=1 -c -o $@ $<

DWTRMWI.BIN: dwterm.c $(OTHEROBJS) dw_coco3fpgawifi.o
	$(CC) $(CFLAGS) -DDW_COCO3FPGAWIFI=1 -DDW_BECKER=1 -o $@ $^ 

DWTERM.dsk: $(BINS)
	rm -f DWTERM.dsk
	decb dskini DWTERM.dsk
	for i in $^; do  decb copy $$i DWTERM.DSK,$$i -b -2; done
	decb dir DWTERM.dsk,


