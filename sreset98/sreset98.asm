%if 0

SRESET98: a simple software-reset (with CTRL+GRPH+Del) utility for PC-98


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

%macro	IOWait	0
		out	5fh, al		; wait approx 1.6us
%endmacro

%define IS_8086		80h

[SECTION .text]

dev_header:
		dw	0ffffh, 0ffffh
dev_attr:	dw	8000h
		dw	Strategy
dev_commands:	dw	Init_Commands
dev_name	db	'$SRESET$'

flags		db	0
pending_reboot	db	0

reqhdr		dd	0
org_09		dd	0

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

New_09:
		pushf
		call	far [cs: org_09]
		push	ax
		push	ds
		xor	ax, ax
		mov	ds, ax
		mov	al, byte [0538h]
		mov	ah, byte [0531h]
		and	ax, 1000011000b		; DEL, CTRL, GRPH
		cmp	ax, 1000011000b
		je	.reset
		pop	ds
		pop	ax
		iret
.reset:
		cli
		mov	byte [cs: pending_reboot], 1
		test	byte [cs: flags], IS_8086
		jnz	reset_8086
		in	al, 35h
		IOWait
		or	al, 10100000b		; SHUT1=1, SHUT1=1
		out	35h, al
		IOWait
		mov	al, 0
		out	0f0h, al
.wait_reset:
		hlt
		jmp	short .wait_reset

reset_8086:
		jmp	0ffffh:0000h


		align	16

dev_bottom:

;----------------------------------------------


Init_Commands:
		pushf
		push	ax
		push	bx
		push	dx
		push	ds
		push	es

		push	cs
		pop	ds
		mov	ax, 3509h
		int	21h
		mov	[org_09], bx
		mov	[org_09 + 2], es
		mov	dx, msg
		mov	ah, 9
		int	21h
		mov	cx, dev_bottom
		pushf
		pop	ax
		cmp	ah, 0f0h
		jb	.l2
		or	byte [flags], IS_8086
		mov	dx, msg_8086
		mov	ah, 9
		int	21h
.l2:
		mov	dx, msg_date
		mov	ah, 9
		int	21h
		mov	word [dev_commands], Commands
		les	bx, [reqhdr]
		mov	byte [es: bx + 13], 1
		mov	word [es: bx + 14], dev_bottom
		mov	word [es: bx + 16], cs
		mov	word [es: bx + 3], 0100h

		mov	dx, New_09
		mov	ax, 2509h
		int	21h
		jmp	short .exit

.err:
		mov	dx, msg_err
		mov 	ah, 9
		int	21h
		mov	ax, 3000h
		int	21h
		cmp	al, 3
		ja	.err_2
		mov	word [cs: dev_attr], 0
		mov	byte [dev_name], 0
		mov	byte [flags], 0
.err_2:
		mov	word [es: bx + 14], 0
		mov	word [es: bx + 3], 810ch
.exit:
		pop	es
		pop	ds
		pop	dx
		pop	bx
		pop	ax
		popf
		retf


msg		db	'SRESET98: simple soft-reset for PC98', '$'
msg_8086	db	' (8086/V30)', '$'
msg_date	db	', built at ', __DATE__, ' ', __TIME__, 13, 10, '$'

msg_err		db	'ERROR: SRESET98 not installed', 13, 10, '$'
