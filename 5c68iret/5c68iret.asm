
%if 0
;----------------------------------------------------------------------------

5C68iret - set dummy `iret' handler into int 5Ch and 68h

author: lpproj (https://github.com/lpproj/)

to build with nasm:
	nasm -f bin -o 5c68iret.sys 5c68iret.asm


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
;segment STACK align=16 class=stack stack


segment _TEXT

		dd	-1
dev_attr	dw	8000h
		dw	Strategy
		dw	Init_Commands
dev_name	db	'$SETIRET'

dev_packet	dd	0

Strategy:
		mov	word [cs: dev_packet], bx
		mov	word [cs: dev_packet + 2], es
		retf

Init_Commands:
		push	ax
		push	bx
		push	cx
		push	dx
		push	ds
		push	es

		lds	bx, [cs: dev_packet]
		mov	word [bx + 3], 810ch
		mov	byte [bx + 13], 0
		mov	word [bx + 14], 0
		mov	word [bx + 16], cs

		push	cs
		pop	ds
		mov	ax, 3000h
		int	21h
		cmp	al, 3
		ja	.l1
		; workaround for DOS 2.x and 3.x: mark as a block device
		xor	ax, ax
		mov	[dev_attr], ax
		mov	[dev_name], al
.l1:
		mov	ah, 52h
		int	21h
		mov	[dosseg], es
		call	main_5c86

		pop	es
		pop	ds
		pop	dx
		pop	cx
		pop	bx
		pop	ax
		retf

put_err:
		push	dx
		mov	dx, errPrefix
		call	put_msg
		pop	dx
put_msg:
		mov	ah, 9
		int	21h
		ret

dosseg		dw	0

check_vect:
		mov	ah, 35h
		int	21h
		mov	dx, es
		push	dx
		mov	al, 3			; al=3: IRET in dosseg
		cmp	dx, [dosseg]
		jne	.l2
		cmp	byte [es: bx], 0cfh
		je	.noerr
.l2:
		dec	al			; al=2: in BIOS ROM (IBMPC)
%if 0						; (but not used)
		cmp	dx, 0ffffh
		je	.l3
		cmp	dx, 0f000h
		jae	.noerr
%endif
.l3:
		dec	al			; al=1: in IO.SYS
		cmp	dx, 60h
		je	.noerr
		dec	al			; al=0: NULL
		or	dx, bx
		jz	.noerr
;.err:
		mov	al, 0ffh
		stc
		jmp	.exit
.noerr:
		clc
.exit:
		pop	dx
		xchg	bx, dx		; bx(seg):dx(off)
		ret

find_iret_vector:
		mov	cl, 2ah
.lp:
		mov	al, cl
		call	check_vect
		cmp	al, 3
		je	.found
		inc	cl
		cmp	cl, 2dh		; 3fh
		jbe	.lp
		stc
		ret
.found:
		clc			; (redundancy)
		ret

main_5c86:
		mov	al, 5ch
		call	check_vect
		mov	dx, errUnknown5c
		cmp	al, 2
		ja	.err
		mov	al, 68h
		call	check_vect
		mov	dx, errUnknown68
		cmp	al, 2
		ja	.err
		call	find_iret_vector
		jc	.err_not_iret_vector
		push	ds
		mov	ds, bx
		mov	ax, 255ch
		int	21h
		mov	ax, 2568h
		int	21h
		pop	ds
		mov	dx, msgVectSet
		call	put_msg
		clc
		ret
.err_not_iret_vector:
		mov	dx, errNoIret
.err:
		call	put_err
		stc
		ret

errPrefix	db	'ERROR:$'
errUnknown5c	db	'someone use vector 5Ch', 13, 10, '$'
errUnknown68	db	'someone use vector 68h', 13, 10, '$'
errNoIret	db	'dummy IRET handler not found', 13, 10, '$'
msgVectSet	db	'dummy IRET handler is installed into vector 5Ch and 68h', 13, 10, '$'

