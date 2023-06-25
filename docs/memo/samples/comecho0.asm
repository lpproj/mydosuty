;
; comecho0 - DOS device driver/application sample: COM style (org 0h)
;
; to build (with nasm):
;   nasm -f bin -o comecho0.com comecho0.asm


		CPU	8086
		BITS	16

; segment _TEXT align=16 public use16 class=CODE
SEGMENT _TEXT


		ORG	0h

devhdr		jmp	short ComStart0		; db 0ebh, ??h
		dw	0ffffh
devattr		dw	8000h
		dw	Strategy
		dw	DevStart
devname		db	'$COMDEV '

;
org_cs		dw	0
reqhdr		dd	0
run_as_device	db	0

;
ComStart0:
		jmp	ComStart

Strategy:
		mov	word [cs: reqhdr], bx
		mov	word [cs: reqhdr + 2], es
		retf

; fix CS:IP to SYS-style (CS:xxxx -> CS+10h:xxxx-100h)
%MACRO		Org100CStoOrg0CS	0
		mov	ax, cs
		mov	[cs: org_cs + 100h], ax	; preserve original CS
		add	ax, 10h
		push	ax			; (push word cs-10h)
		mov	ax, .new_ip
		push	ax			; (push word offset .new_ip)
		retf				; jmp cs-10h : .new_ip
.new_ip:
%ENDMACRO

DevStart:	; (InitCommands)
		push	ax
		push	bx
		push	cx
		push	dx
		push	si
		push	di
		push	ds
		push	es
		push	cs
		pop	ds
		mov	byte [run_as_device], 1
		les	bx, [reqhdr]
		les	di, [es: bx + 18]	; es:di = device param string
; skip argv0
.skip0_lp:
		mov	al, [es: di]
		call	IsCRLF
		je	.scan
		cmp	al, 9
		je	.scan
		inc	di
		cmp	al, ' '		; skip first space (other than NEC98 MS-DOS)
		je	.scan
		cmp	al, 0		; skip first null char between devfile and parameter string (NEC98 MS-DOS)
		je	.scan
		jmp	short .skip0_lp
.scan:
		mov	cx, di
.scan1_lp:
		mov	al, [es: di]
		call	IsCRLF
		je	.scan1_brk
		inc	di
		jmp	short .scan1_lp
.scan1_brk:
		xchg	di, cx
		sub	cx, di			; cx = length, es:di = top of args

		call	CommonStart

; end of init device - just exit without resident
		push	cs
		pop	ds
		mov	ax, 3000h
		int	21h
		cmp	al, 5
		jae	.dos5
.dos3:
		mov	word [devattr], 0		; fake as block device
		mov	byte [devname], 0		; no unit
.dos5:
		mov	word [devhdr], 0ffffh	; fix device chain (ffff:ffff = end of chain)
		lds	bx, [reqhdr]
		mov	byte [bx + 13], 0
		mov	word [bx + 14], 0
		;mov	word [bx + 14], TsrBottom
		mov	ax, cs
		mov	word [bx + 16], ax
		mov	word [bx + 3], 0100h		; DONE

		pop	es
		pop	ds
		pop	di
		pop	si
		pop	dx
		pop	cx
		pop	bx
		pop	ax
		retf

IsCRLF:
		cmp	al, 13
		je	.brk
		cmp	al, 10
.brk:
		ret


;
CommonStart:
		mov	dx, msgRunCom
		cmp	byte [run_as_device], 0
		je	.l2
		mov	dx, msgRunDevice
.l2:
		mov	ah, 9
		int	21h
.lp:
		mov	al, [es: di]
		call	IsCRLF
		je	.brk
		mov	dl, al
		mov	ah, 2
		int	21h
		inc	di
		jmp	short .lp
.brk:
		mov	dx, msgCRLF
		mov	ah, 9
		int	21h
		mov	al, 0
		ret


;
ComStart:
		Org100CStoOrg0CS
		push	cs
		pop	ds		; mov ds, cs
		mov	ah, 51h		; DOS 2+: Get Current PSP
		int	21h
		mov	es, bx		; es = current PSP (not needed on .COM)
		mov	di, 81h		; es:di = offset of cmd string (PSP:0081h)
		xor	ch, ch
		mov	cl, [es: 80h]	; cx = cmd length (byte [PSP:0080h])

		; treat `unreliable' command line
		; (do not trust the parameter set by the parent)
		cmp	cl, 127
		jbe	.cfix_cr
		mov	cl, 127
.cfix_cr:
		;cmp	cl, 127
		jae	.cfix_brk
		push	di
		add	di, cx
		mov	byte [es: di], 13
		pop	di
.cfix_brk:
		; skip first space character
		jcxz	.skipsp_brk
		cmp	byte [es: di], ' '
		jne	.skipsp_brk
		inc	di
		dec	cx
.skipsp_brk:

		call	CommonStart

		mov	ah, 4ch
		int	21h

msgRunCom	db	'Program:', '$'
msgRunDevice	db	'Device:', '$'
msgCRLF		db	13, 10, '$'

