AS=nasm
HOST_CC=gcc
CC=x86_64-w64-mingw32-gcc
CFLAGS=-Wall -O2 -fno-omit-frame-pointer
#CC=i686-w64-mingw32-gcc
#CFLAGS=-Wall -O2 -march=i386 -mtune=generic
LDFLAGS=-s -static

RM=rm -f

D=/
O=o
HOST_X=.exe

BASEDIR=..$(D)



all: format2hd.exe

clean:
	-$(RM) *.$(O) format2hd.exe
	-$(RM) bin2list$(HOST_X) bin2list bin2list.$(O)
	-$(RM) bootdumy.bin

format2hd.exe: $(BASEDIR)format2hd.c $(BASEDIR)bootdumy.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BASEDIR)bootdumy.h: $(BASEDIR)bootdumy.asm $(BASEDIR)bin2list.c
	$(HOST_CC) -o bin2list$(HOST_X) $(BASEDIR)bin2list.c
	$(AS) -f bin -o bootdumy.bin $(BASEDIR)bootdumy.asm
	.$(D)bin2list bootdumy.bin $@

