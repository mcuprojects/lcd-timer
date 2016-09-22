CC=sdcc
IHXTOHEX=packihx

TARGET=timer

all: hex

ihx: $(TARGET).c
	$(CC) $(TARGET).c

hex: ihx
	$(IHXTOHEX) $(TARGET).ihx > $(TARGET).hex

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

install: all
# TODO
