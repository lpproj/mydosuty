%if 0
;----------------------------------------------------------------------------

NOSHARE - suppress "WARNING! SHARE should be loaded for large media" message on booting MS-DOS 4.0

author: lpproj (https://github.com/lpproj/)

to build with nasm:
	nasm -f bin -o noshare.com noshare.asm

usage: edit config.sys to add INSTALL statement (after all INSTALL statements) to install noshare.com temporarily

	INSTALL=noshare.com

(noshare.com will be removed from memory by itself before loading command.com)


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

SEGMENT		_TEXT

		ORG	100h
mem_org100h:
		jmp	short Init

myPSP		dw	0

New_2F:
		cmp	ax, 1000h
		je	new2f10
.chain:
		db	0eah			; jmp xxxx:yyyy
org_2F		dd	0

new2f10:
		push	ds
		push	cs
		pop	ds
		mov	al, 0ffh		; share (dummy) installed
		push	ax
		push	es
		xor	ax, ax
		mov	es, ax
		mov	ax, cs
		cmp	ax, [es: 2fh * 4 + 2]
		jne	.exit1
		cmp	word [es: 2fh * 4], New_2F
		jne	.exit1
		cli
		mov	ax, [org_2F]
		mov	[es: 2fh * 4], ax
		mov	ax, [org_2F + 2]
		mov	[es: 2fh * 4 + 2], ax
		mov	ax, [myPSP]
		dec	ax
		mov	es, ax
		mov	word [es: 1], 0	; mark myself as free MCB
		mov	word [es: 10h], 0	; erase first word in PSP (to be safe)
.exit1:
		pop	es
		pop	ax
		pop	ds
		iret

		ALIGN	16
TSR_Bottom:
;------------------------------

Init:
		push	cs
		pop	ds
		mov	[myPSP], es
		mov	ax, 3000h
		int	21h
		cmp	al, 4
		je	.l2		; needed only for DOS 4.0...
		mov	dx, msgNoDOS4
.no_tsr:
		mov	ah, 9
		int	21h
		mov	ax, 4c00h
		int	21h
.l2:
		mov	ax, 1000h	; share installation check
		int	2fh
		cmp	al, 0
		mov	dx, msgShareExist
		jne	.no_tsr

		mov	ax, 352fh
		int	21h
		mov	[org_2F], bx
		mov	[org_2F + 2], es

		; free environment variable MCB if exist
		mov	es, [myPSP]
		xor	bx, bx
		xchg	bx, [es: 002ch]
		test	bx, bx
		jz	.tsr
		push	bx	; validate environment MCB (to be safe)
		dec	bx
		mov	es, bx
		mov	al, [es: 0]
		mov	dx, [es: 1]
		pop	bx
		cmp	dx, [myPSP]
		jne	.tsr
		cmp	al, 'M'
		je	.freeenv
		cmp	al, 'Z'
		jne	.tsr
.freeenv:
		mov	es, bx
		mov	ah, 49h
		int	21h
.tsr:
		mov	ax, 252fh
		mov	dx, New_2F
		int	21h
		mov	ax, 3100h
		mov	dx, ((TSR_Bottom - mem_org100h) + 100h + 15) / 16
		int	21h


msgNoDOS4	db	'NOSHARE: only for DOS 4.0', 13, 10, '$'
msgShareExist	db	'NOSHARE: SHARE.EXE already installed', 13, 10, '$'
msgBrokenSDA	db	'NOSHARE: broken SDA', 13, 10, '$'

