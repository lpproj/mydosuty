;
; nulldev - a sample of NUL-like device driver
;
; to build (with nasm):
;   nasm -f bin -o nulldev.sys nulldev.asm


%ifdef DEFAULT_NUL_DEVICE
	%define NULLDEV_ATTR	8004h
	%define NULLDEV_NAME	'NUL     '
%else
	%define NULLDEV_ATTR	8000h
	%define NULLDEV_NAME	'NULL    '
%endif

		CPU	8086
		BITS	16

; segment _TEXT align=16 public use16 class=CODE
SEGMENT _TEXT

		dd	-1
devattr		dw	NULLDEV_ATTR
devstrategy	dw	InitStrategy
devcommand	dw	InitCommands
devname		db	NULLDEV_NAME

devid		db	'$nulldev00'
devid_end:

reqhdr		dd	0

Strategy:
		mov	[cs: reqhdr], bx
		mov	[cs: reqhdr + 2], es
		retf

Commands:
		push	ax
		push	bx
		push	di
		push	ds
		push	es
		push	cs
		pop	ds		; mov ds, cs
		les	di, [reqhdr]
		xor	bx, bx
		mov	[es: di + 3], bx		; (clear status code)
		mov	bl, [es: di + 2]		; get command code
		add	bx, bx
		cmp	bx, commands_tbl_end - commands_tbl
		jb	.do_cmd
		call	commands_nosup
		jmp	short .after_cmd
.do_cmd:
		call	[bx + commands_tbl]
.after_cmd:
		jc	.err
		or	word [es: di + 3], 0100h	; DONE
		jmp	.exit_2
.err:
		mov	ah, 81h
		mov	word [es: di + 3], ax		; ERROR, DONE, al=errcode
.exit_2:
		pop	es
		pop	ds
		pop	di
		pop	bx
		pop	ax
		retf

commands_tbl:
		dw	commands_nosup		; func  0 INIT
		dw	commands_nosup		; func  1 MEDIA CHECK
		dw	commands_nosup		; func  2 BUILD BPB
		dw	commands_nosup		; func  3 IOCTL INPUT
		dw	commands_read		; func  4 INPUT
		dw	commands_ndread		; func  5 NON-DESTRUCTIVE INPUT NO WAIT
		dw	commands_done		; func  6 INPUT STATUS
		dw	commands_done		; func  7 INPUT FLUSH
		dw	commands_write		; func  8 OUTPUT
		dw	commands_write		; func  9 OUTPUT WITH VERIFY
		dw	commands_done		; func 10 OUTPUT STATUS
		dw	commands_done		; func 11 OUTPUT FLUSH
		dw	commands_nosup		; func 12 IOCTL INPUT
commands_tbl_end:

commands_nosup:
		mov	al, 3
		stc
		ret

commands_read:
		mov	word [es: di + 18], 0	; read zero byte
		clc
		ret

commands_write:
				; do not modify write count (so all data will be `processed')
commands_done:
		clc		; just DONE it
		ret

commands_ndread:
		or	word [es: di + 3], 0200h	; BUSY (no char in buffer)
		clc
		ret


TsrBottom:
;--------------------------------------


init_reqhdr	dd	0

InitStrategy:
		mov	[cs: init_reqhdr], bx
		mov	[cs: init_reqhdr + 2], es
		retf

;
InitCommands:
		pushf
		push	ax
		push	bx
		push	dx
		push	ds
		push	es
		cld
		push	cs
		pop	ds		; mov ds, cs
		call	GetDosVer
		mov	[dos_ver], ax
		mov	dx, msgNullDev
		mov	ah, 9
		int	21h
		mov	word [devstrategy], Strategy	; release InitStrategy
		mov	word [devcommand], Commands	; release InitCommands
		call	CheckTheDriver
		jc	.install
;.notinstall:
		mov	dx, errAlready
		mov	ah, 9
		int	21h
		cmp	byte [dos_major], 5
		jae	.notinst_dos5
;.notinst_dos3:
		mov	word [devattr], 0		; fake as block device
		mov	byte [devname], 0		; no unit
.notinst_dos5:
		lds	bx, [init_reqhdr]
		mov	byte [bx + 13], 0
		mov	word [bx + 14], 0
		jmp	short .inst_exit
.install:
		lds	bx, [init_reqhdr]
		mov	word [bx + 14], TsrBottom
.inst_exit:
		mov	word [bx + 16], cs
		mov	word [bx + 3], 0100h		; DONE
		pop	es
		pop	ds
		pop	dx
		pop	bx
		pop	ax
		popf
		retf
;
GetDosVer:
		push	bx
		push	cx
		mov	ax, 3000h	; Get DOS Version
		int	21h
		cmp	al, 10
		jne	.exit
		mov	ax, 1e03h	; OS/2 1.x as DOS 3.30
.exit:
		xchg	al, ah
		pop	cx
		pop	bx
		ret

dos_ver:
dos_minor	db	0
dos_major	db	0

;
GetNulDevice:
		;mov	bx, 0ffffh - 22h
		mov	ah, 52h
		int	21h
		mov	ax, [dos_ver]
		cmp	ah, 2
		je	.dos2x
		cmp	ax, 0300h
		je	.dos3_0
		; dos 3.1 or later
		add	bx, 22h
		ret
.dos2x:		; dos 2.x
		add	bx, 17h
		ret
.dos3_0:	; dos 3.0
		add	bx, 28h
		ret

;
CheckTheDriver:
		push	ax
		push	cx
		push	si
		push	di
		push	ds
		call	GetNulDevice
		mov	ax, cs
		mov	ds, ax
		;cld
.lp:
		cmp	bx, 0ffffh
		je	.brk
		mov	si, devid
		lea	di, [bx + devid]
		; skip myself
		mov	cx, es
		cmp	ax, cx
		jne	.check
		cmp	si, di
		je	.next
.check:
		mov	cx, devid_end - devid
		repe	cmpsb
		je	.brk
.next:
		les	bx, [es: bx]
		jmp	short .lp
.brk:
		mov	ax, 1
		add	ax, bx		; set CF if bx==ffffh
		pop	ds
		pop	di
		pop	si
		pop	cx
		pop	ax
		ret


msgNullDev	db	'NULLDEV - sample driver by lpproj, 2023.', 13, 10, '$'
errAlready	db	'ERROR: NULLDEV already installed.', 13, 10, '$'

