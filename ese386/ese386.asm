
%if 0
;----------------------------------------------------------------------------

ESE386 - yet another nise386

author: lpproj (https://github.com/lpproj/)

to build with nasm and wlink (OpenWatcom):
	nasm -f obj -o ese386.obj ese386.asm
	wlink name ese386.exe form dos op nodefaultlib f ese386.obj


This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

%endif


	CPU	8086
	BITS	16

segment _TEXT
segment STACK align=16 class=stack stack


segment _TEXT

	dd	-1
	dw	0000h
	dw	Strategy
	dw	Commands
	times 8 db 0

dev_packet	dd	0

Strategy:
	mov	word [cs: dev_packet], bx
	mov	word [cs: dev_packet + 2], es
	retf

Commands:
	push	bx
	push	ds
	lds	bx, [cs: dev_packet]
	mov	word [bx + 3], 810ch
	mov	byte [bx + 13], 0
	mov	word [bx + 14], 0
	mov	word [bx + 16], cs
	call	ESE386
	pop	ds
	pop	bx
	retf


;
; Set CPU Type (0000:0480 bit1-0)
; Reference: UNDOCUMENTED 9801/9821 Vol.2 memsys.txt
;            http://www.webtech.co.jp/company/doc/undocumented_mem/
 
ESE386:
	xor	bx, bx
	mov	ds, bx
%ifdef   ESEV30
	and	byte [0480h], 0fch
%elifdef ESE286
	and	byte [0480h], 0fch
	or	byte [0480h], 1
%else    ; ESE386
	or	byte [0480h], 3
%endif
	ret


..start:
	call	ESE386
	mov	ax, 4c00h
	int	21h


segment STACK
	resw	64

