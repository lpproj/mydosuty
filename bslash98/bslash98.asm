%if 0
;----------------------------------------------------------------------------

BSLASH98 - a minimal MS-DOS TSR to replace Yen-sign (0x5c) to backslash (0xfc)
           on NEC PC-98x1

author: lpproj (https://github.com/lpproj/)

to build with nasm:
    nasm -f bin -o bslash98.com bslash98.asm

usage:
    bslash98      (to stay)
    bslash98 /r   (to release)

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


segment _TEXT

		CPU	8086
		BITS	16

		ORG	100h

entry:
		jmp	init

		ALIGN	4

sign_top:
		db	'$BSLASH98$00'
sign_end:

org_29		dd	0

New_29:
		pushf
		push	ds
		mov	ds, [cs: IOSEG]
		cmp	byte [008ah], 0	; check GRAPH mode
		je	.single
		cmp	byte [cs: leadchar], 0
		jne	.fallthru_d		; do not replace the trailing 0x5c in a DBC
		cmp	al, 81h
		jae	.ck0
.single:
		cmp	al, 5ch			; yen sign?
		je	.cswap
.fallthru_d:
		mov	byte [cs: leadchar], 0
.fallthru:
		pop	ds
		popf
		jmp	far [cs: org_29]
.ck0:
		cmp	al, 9fh
		jbe	.dbc
		cmp	al, 0e0h
		jb	.fallthru
		cmp	al, 0fch
		ja	.fallthru
.dbc:
		mov	[cs: leadchar], al
		jmp	short .fallthru
.cswap:
		push	ax
		mov	ah, 0
		mov	[cs: leadchar], ah
		xchg	ah, byte [008ah]	; save GRAPH/KANJI state and set GRAPH mode
		mov	al, 0fch		; backslash
		pushf
		call	far [cs: org_29]
		xchg	ah, byte [008ah]	; restore GRAPH/KANJI state
		pop	ax
		pop	ds
		popf
		iret

IOSEG		dw	0060h
leadchar	db	0

		ALIGN	16
tsr_bottom:
; --------------------------------------------------------------

optHelp		db	0
optR		db	0

init:
		mov	[current_psp], ds	; com
		xor	ax, ax
		xchg	ax, [002ch]		; get env seg
		test	ax, ax
		jz	.l0_0
		mov	es, ax
		mov	ah, 49h			; release env seg
		int	21h
.l0_0:
		mov	ax, 3529h
		int	21h
		mov	[org_29], bx
		mov	[org_29 + 2], es
		cld
		xor	cx, cx
		mov	si, 0080h
		lodsb
		mov	cl, al
		jcxz	init_main
.l_cmdlp:
		lodsb
		cmp	al, '-'
		je	.l_opt
		cmp	al, '/'
		je	.l_opt
		loop	.l_cmdlp
		jmp	short init_main
.l_opt:
		jcxz	init_main
		lodsb
		cmp	al, 'H'
		je	.l_opt_h
		cmp	al, 'h'
		je	.l_opt_h
		cmp	al, '?'
		jne	.l_opt2
.l_opt_h:
		mov	byte [optHelp], 1
.l_opt2:
		cmp	al, 'R'
		je	.l_opt_r
		cmp	al, 'r'
		jne	.l_opt_next
.l_opt_r:
		mov	byte [optR], 1
.l_opt_next:
		loop	.l_cmdlp

init_main:
		cmp	byte [optR], 0
		jne	init_release
init_tsr:
		call	find_tsr
		mov	dx, err_exist
		jnc	errexit
		mov	dx, New_29
		mov	ax, 2529h
		int	21h
		mov	dx, msg_install
		call	putmsg_n
		mov	dx, tsr_bottom
		mov	cl, 4
		shr	dx, cl
		mov	ax, 3100h
		int	21h
errexit:
		call	putmsg_n
		mov	ax, 4c01h
		int	21h
init_release:
		call	find_tsr
		mov	dx, err_notexist
		jc	errexit
		mov	dx, err_hooked
		mov	ax, es
		cmp	ax, [org_29 + 2]	; check int29h overriden
		jne	errexit
		push	ds
		lds	dx, [es: org_29]
		mov	ax, 2529h		; restore int29h
		int	21h
		pop	ds
		not	byte [es: sign_top]	; waste signature in the tsr (to be safe)
		mov	ah, 49h		; release tsr memblk
		int	21h
		mov	dx, msg_release
exit_normal:
		call	putmsg_n
		mov	ax, 4c00h
		int	21h

putmsg_n:
		mov	ah, 9
		int	21h
put_n:
		push	ax
		push	dx
		mov	dl, 13
		mov	ah, 2
		int	21h
		mov	dl, 10
		mov	ah, 2
		int	21h
		pop	dx
		pop	ax
		ret

current_psp		dw	0

find_tsr:
		mov	ax, 5802h
		int	21h
		cmp	al, 1
		jbe	.l_00
		mov	al, 0		; (UMB not linked if func 5802h is not supported)
.l_00:
		xor	ah, ah
		push	ax		; push UMB link state
		mov	ax, 5803h
		mov	bx, 1
		int	21h		; link UMB

		mov	ah, 52h
		int	21h
		mov	dx, [es: bx - 2]	; 1st MCB
.lp:
		mov	es, dx
		mov	ax, [es: 1]
		dec	ax
		cmp	ax, dx			; owner?
		jne	.l_next
		inc	ax
		cmp	ax, [current_psp]
		je	.l_next
		mov	si, sign_top
		lea	di, [sign_top + 16]
		mov	cx, sign_end - sign_top
		repe	cmpsb
		je	.l_found
.l_next:
		mov	al, [es: 0]
;		cmp	al, 'Z'
;		je	.l_notfound
		cmp	al, 'M'
		jne	.l_notfound
		add	dx, [es: 3]
		inc	dx
		jmp	short .lp
.l_notfound:
		mov	dx, 0ffffh
.l_found:
		mov	ax, 5803h
		pop	bx		; pop previous UMB link state
		int	21h
		add	dx, 1		; dx=0,CF=1 if not found
		mov	es, dx
		ret


err_exist	db	'BSLASH98 already installed$'
msg_install	db	'BSLASH98 installed$'
err_notexist	db	'BSLASH98 not installed$'
msg_release	db	'BSLASH98 released$'
err_hooked	db	'BSLASH98 not released (int 29h is hooked by other program)$'

