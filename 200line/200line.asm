
%if 0
;----------------------------------------------------------------------------

200line - feel like 200line mode for NEC PC-98

author: lpproj (https://github.com/lpproj/)

to build with nasm:
	nasm -f bin -o 200line.sys 200line.asm


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

	ORG	0

dev_header:
	dd	-1
	dw	8000h
	dw	Strategy
dev_commands:
	dw	Init_Commands
	db	"$200LINE"

dev_packet	dd	0

Strategy:
	mov	word [cs: dev_packet], bx
	mov	word [cs: dev_packet + 2], es
	retf

Commands:
	push	bx
	push	ds
	lds	bx, [cs: dev_packet]
	mov	word [bx + 3], 8103h
	pop	ds
	pop	bx
	retf


New18:
	pushf
	cmp	ah, 0ah
	je	new18_wrap
.popf_and_chain:
	popf
.chain:
	db	0eah	; jmp oooo:ssss
org18	dd	0

new18_wrap:
	call	far [cs: org18]
	push	ax
	mov	al, 6
	out	68h, al
	pop	ax
	iret

dev_bottom:
;--------------------------------------

Init_Commands:
	push	ax
	push	bx
	push	dx
	push	ds

	push	cs
	pop	ds
	mov	ah, 09h
	mov	dx, msgOpening
	int	21h

	mov	word [dev_commands], Commands

	push	es
	mov	ax, 3518h
	int	21h
	mov	word [org18], bx
	mov	word [org18 + 2], es
	pop	es
	mov	ax, 2518h
	mov	dx, New18
	int	21h

	lds	bx, [dev_packet]
	mov	word [bx + 3], 0100h
	mov	byte [bx + 13], 1
	mov	word [bx + 14], (dev_bottom - dev_header + 15) & 0ffffh
	mov	word [bx + 16], cs

	mov	al, 6
	out	68h, al

	pop	ds
	pop	dx
	pop	bx
	pop	ax
	retf

msgOpening:
	db	"200line.sys built at ", __DATE__, 13, 10, '$'

