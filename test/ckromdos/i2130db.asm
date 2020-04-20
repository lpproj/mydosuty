%if 0	; %comment

i2130db.asm: hook int 21h,ax=30DBh and display register usage
(to check Datalight ROM-DOS revision)

to build:
	nasm -f bin -o i2130db.sys i2130db.asm


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

%endif	; %endcomment


	BITS	16
	CPU	8086

segment _TEXT

	ORG	0

	dd	-1
	dw	8000h
	dw	Strategy
commands_ptr:
	dw	Init_Commands
	db	'$2130DB$'

reqhdr:
	dd	0
org21:
	dd	0


New21:
	pushf
	cmp	ax, 30dbh
	je	new21_rdver
.org:
	popf
	jmp	far [cs: org21]

new21_rdver:
	mov	word [cs: s_header], s_0
	call	DispRegs
	call	far [cs: org21]
	pushf
	cmp	bx, 1d06h
	jne	.l0
;	mov	bx, 1d08h
.l0:
	popf
	mov	word [cs: s_header], s_1
	call	DispRegs
	retf	2


DispRegs:
	pushf		; +20
	push	ax	; +18
	push	cx	; +16
	push	dx	; +14
	push	bx	; +12
	push	sp	; +10
	push	bp	; +8
	push	si	; +6
	push	di	; +4
	push	ds	; +2
	push	es	; +0
	mov	bp, sp
	push	cs
	pop	ds
	cld
;
	mov	si, [s_header]
	call	puts
	mov	si, s_ax
	call	puts
	mov	ax, [bp + 18]
	call	puth16
	mov	si, s_bx
	call	puts
	mov	ax, [bp + 12]
	call	puth16
	mov	si, s_cx
	call	puts
	mov	ax, [bp + 16]
	call	puth16
	mov	si, s_dx
	call	puts
	mov	ax, [bp + 14]
	call	puth16
	mov	si, s_si
	call	puts
	mov	ax, [bp + 6]
	call	puth16
	mov	si, s_di
	call	puts
	mov	ax, [bp + 4]
	call	puth16
	mov	si, s_ds
	call	puts
	mov	ax, [bp + 2]
	call	puth16
	mov	si, s_es
	call	puts
	mov	ax, [bp]
	call	puth16
	call	putnl
;
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	bp
	pop	bx	; drop sp
	pop	bx
	pop	dx
	pop	cx
	pop	ax
	popf
	ret

s_header	dw	s_0

s_0	db	'call', 0
s_1	db	'ret ', 0
s_ax	db	' AX:', 0
s_bx	db	' BX:', 0
s_cx	db	' CX:', 0
s_dx	db	' DX:', 0
s_si	db	' SI:', 0
s_di	db	' DI:', 0
s_ds	db	' DS:', 0
s_es	db	' ES:', 0

;--------------------------------------
%macro	PutC	0
	int	29h
%endmacro

putnl:
	mov	al, 13
	PutC
	mov	al, 10
	PutC
	ret

puts_0:
	PutC
puts:
	lodsb
	cmp	al, 0
	jne	puts_0
	ret

puth4:
	and	al, 0fh
	add	al, '0'
	cmp	al, '9'
	jbe	.l2
	add	al, 'A' - '9' - 1
.l2:
	PutC
	ret

puth8:
	push	ax
	mov	ah, al
	shr	al, 1
	shr	al, 1
	shr	al, 1
	shr	al, 1
	call	puth4
	xchg	al, ah
	call	puth4
	pop	ax
	ret

puth16:
	xchg	al, ah
	call	puth8
	xchg	al, ah
	jmp	puth8


Strategy:
	mov	[cs: reqhdr], bx
	mov	[cs: reqhdr + 2], es
	retf

Commands:
	push	bx
	push	ds
	lds	bx, [cs: reqhdr]
	mov	word [bx + 3], 8103h
	pop	ds
	pop	bx
	retf



	align 16
dev_bottom:



Init_Commands:
	push	ax
	push	bx
	push	dx
	push	ds
	push	es
	;
	push	cs
	pop	ds
	mov	ax, 3521h
	int	21h
	mov	[org21], bx
	mov	[org21 + 2], es
	les	bx, [reqhdr]
	mov	word [es: bx + 3], 0100h
	mov	byte [es: bx + 13], 1
	mov	word [es: bx + 14], dev_bottom
	mov	[es: bx + 16], cs
	mov	dx, msgInit
	mov	ah, 9
	int	21h
	mov	ax, 2521h
	mov	dx, New21
	int	21h
	pop	es
	pop	ds
	pop	dx
	pop	bx
	pop	ax
	retf

msgInit:
	db	'i2130db.sys installed.', 13, 10, '$'

