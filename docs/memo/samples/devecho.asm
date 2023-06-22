;
; devecho - DOS device driver sample: put device=... string and exit (without resident)
;
; to build (with nasm):
;   nasm -f bin -o devecho.sys devecho.asm

		CPU	8086
		BITS	16

; segment _TEXT align=2 public use16 class=CODE
SEGMENT _TEXT

		ORG	0


		dd	-1
devattr		dw	8000h
		dw	Strategy
		dw	Commands
devname		db	'$DEVECHO'

reqhdr		dd	0

Strategy:
		mov	[cs: reqhdr], bx
		mov	[cs: reqhdr + 2], es
		retf

Commands:
		push	bx
		push	ds
		lds	bx, [cs: reqhdr]
		cmp	byte [bx + 2], 0		; INIT command?
		jne	.noinit
		call	Command_Init
		jmp	short .exit
.noinit:
		mov	word [bx + 3], 8103h		; error, done, errcode=3
.exit:
		pop	ds
		pop	bx
		retf

;
Command_Init:
		pushf
		push	ax
		push	cx
		push	dx
		push	si
		push	es
		cld			; clear direction flag (to be safe)

		lds	si, [bx + 18]	; ds:si = device command string (terminated with CR)
		call	PutCmdLine
		call	PutCRLF

		mov	ax, 3000h	; check DOS version
		int	21h
		cmp	al, 10
		je	.dos3		; OS/2 1.x returns DOS 10.x
		cmp	al, 5
		jae	.dos5
.dos3:
		; DOS2/3/4: terminate device driver without resident
		;         (supported only block device)
		mov	word [cs: devattr], 0		; fake as block device
		mov	byte [cs: devname], 0		; no unit
.dos5:
		; DOS5+: terminate device driver without resident
		;   unit = 0 (blockdev)
		;   end address = device header (cs:0000)
		lds	bx, [cs: reqhdr]
		mov	byte [bx + 13], 0
		mov	word [bx + 14], 0
		mov	word [bx + 16], cs
		mov	word [bx + 3], 0100h		; DONE
.exit:

		pop	es
		pop	si
		pop	dx
		pop	cx
		pop	ax
		popf
		ret

PutCmdLineN:
		jcxz	.exit
.lp:
		lodsb
		cmp	al, 20h
		jae	.put1
.phex:
		push	ax
		mov	al, '['
		call	PutC
		pop	ax
		push	ax
		call	PutH8
		mov	al, ']'
		call	PutC
		pop	ax
		jmp	short .cont
.put1:
		call	PutC
.cont:
		cmp	al, 13
		je	.exit
		cmp	al, 10
		je	.exit
		loop	.lp
.exit:
		ret

PutCmdLine:
		push	cx
		mov	cx, 0ffffh
		call	PutCmdLineN
		pop	cx
		ret

PutC:
	%if 1
		push	ax
		push	dx
		mov	ah, 2
		mov	dl, al
		int	21h
		pop	dx
		pop	ax
		ret
	%else
		; to stdout handle: when you don't want to extract TAB to spaces...
		push	bx
		push	cx
		push	dx
		push	ds
		push	ax
		mov	dx, sp
		push	ss
		pop	ds
		mov	bx, 1
		mov	cx, 1
		mov	ah, 40h
		int	21h
		pop	ax
		pop	ds
		pop	dx
		pop	cx
		pop	bx
		ret
	%endif


PutH8:
		push	ax
		shr	al, 1
		shr	al, 1
		shr	al, 1
		shr	al, 1
		call	.put_h4
		pop	ax
		push	ax
		and	al, 0fh
		call	.put_h4
		pop	ax
		ret
.put_h4:
		add	al, '0'
		cmp	al, '9'
		jbe	.put_h4_2
		add	al, 'A' - '9' - 1
.put_h4_2:
		jmp	short PutC


PutCRLF:
		push	ax
		mov	al, 13
		call	PutC
		mov	al, 10
		call	PutC
		pop	ax
		ret


