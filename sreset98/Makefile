
RM=rm -f
#RM=del
ASFLAG_DEBUG=-DDEBUG -l $*.lst

all: sreset98.sys

clean:
	-$(RM) sreset98.sys
	-$(RM) sreset98.lst

sreset98.sys: sreset98.asm
	nasm -f bin $(ASFLAG_DEBUG) -o $@ $<

