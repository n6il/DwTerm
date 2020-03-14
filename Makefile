CC = cmoc
CFLAGS =

OTHEROBJS = hirestxt.o font4x8.o writeCharAt_51cols.o
BINS = DWTRMBCK.BIN DWTRMBB.BIN DWTRMBB1.BIN DWTRM232.BIN DWTRMWI.BIN \
	DWTRMB63.BIN DWTBCKLT.BIN DWTBB1LT.BIN

all: DWTERM.dsk DWTERM.zip
	

clean:
	rm -f *.o *.s *.lst *.map *.i
	rm -f DWT*.BIN
	rm -f DWT*.CAS
	rm -f DWT*.WAV
	rm -f DWTERM.dsk
	rm -f DWTERM.zip

dw_becker.o: drivewire.c dwread.asm dwwrite.asm
	$(CC) $(CFLAGS) -DDW_BECKER=1 -c -o $@ $<

DWTRMBCK.BIN: dwterm.c $(OTHEROBJS) dw_becker.o
	$(CC) $(CFLAGS) -DDW_BECKER=1 -o $@ $^ 

DWTBCKLT.BIN: dwterm.c dw_becker.o
	$(CC) $(CFLAGS) --org=1800 -DLITE=1 -DDW_BECKER=1 -o $@ $^
	bin2cas -r 44100 -C -n DWTERM -l 0x1800 -e 0x1800 -o $(@:BIN=WAV) $@
	bin2cas -C -n DWTERM -l 0x1800 -e 0x1800 -o $(@:BIN=CAS) $@

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

DWTBB1LT.BIN: dwterm.c dw_bb1.o
	$(CC) $(CFLAGS) --org=1800 -DLITE=1 -DDW_BAUD38400=1 -o $@ $^
	bin2cas -r 44100 -C -n DWTERM -l 0x1800 -e 0x1800 -o $(@:BIN=WAV) $@
	bin2cas -C -n DWTERM -l 0x1800 -e 0x1800 -o $(@:BIN=CAS) $@

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
	for i in $^; do  decb copy $$i DWTERM.dsk,$$i -b -2; done
	decb dir DWTERM.dsk,

DWTERM.zip: $(BINS) DWTERM.dsk
	zip $@ *.BIN *.CAS *.WAV DWTERM.dsk
