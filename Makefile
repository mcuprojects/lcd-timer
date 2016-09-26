CC=sdcc

SERIALPORT=/dev/ttyUSB0
BAUDRATE=115200

TARGET=timer

all: hex bin

ihx: $(TARGET).c
	$(CC) $(TARGET).c

hex: ihx
	packihx $(TARGET).ihx > $(TARGET).hex
	
bin: ihx
	hex2bin $(TARGET).ihx

clean:
	rm -f $(TARGET).map
	rm -f $(TARGET).mem
	rm -f $(TARGET).asm
	rm -f $(TARGET).lst
	rm -f $(TARGET).rel
	rm -f $(TARGET).rst
	rm -f $(TARGET).sym
	rm -f $(TARGET).lnk
	rm -f $(TARGET).ihx
	rm -f $(TARGET).hex
	rm -f $(TARGET).bin

install: ihx
	stcgal -p $(SERIALPORT) -P stc12 -b $(BAUDRATE) $(TARGET).ihx
