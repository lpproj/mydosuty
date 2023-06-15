
.ERASE

RM=rm -f
CC=*wcl386
CFLAGS=-s -zq -Fr -bt=dos -3r -bt=dos

COMPILE=$(CC) $(CFLAGS) -c -Fo=$^*.obj $[@

# format: MP
#BINTYPE=format pharlap
# format: P3
BINTYPE=format pharlap ext

LDFLAGS=op q op caseexact $(BINTYPE) op stack=32k

all: dosxinfo.exp dosxselv.exp

clean: .SYMBOLIC
	-$(RM) dosxinfo.exp dosxinfo.map
	-$(RM) dosxselv.exp dosxselv.map
	-$(RM) *.obj *.err

dosxinfo.exp: dosxinfo.asm
	$(COMPILE)
	*wlink $(LDFLAGS) name $^@ file { dosxinfo.obj }

dosxselv.exp: dosxinfo.asm
	$(COMPILE) -DVIEW_SEL=1
	*wlink $(LDFLAGS) name $^@ file { $^*.obj }

