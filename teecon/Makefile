# makefile for wmake (bundled with OpenWatcom)

RM_F=rm -f
CP=cp

O=obj
X=.exe

CC16 = wcl
CFLAGS16 = -zq -fr -s -os -ms


all: teecon$(X)

teecon$(X): teecon.c
	$(CC16) -Fe=$@ $(CFLAGS16) teecon.c

clean:
	-$(RM_F) *.$(O) *.err
	-$(RM_F) teecon$(X)

