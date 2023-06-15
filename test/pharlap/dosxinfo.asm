;
; dosxinfo: Get (minimal) information about PharLap 386/Extender
;
; To build (with OpenWatcom):
;   wmake
;
; This is free and unencumbered software released into the public domain.
;
; Anyone is free to copy, modify, publish, use, compile, sell, or
; distribute this software, either in source code form or as a compiled
; binary, for any purpose, commercial or non-commercial, and by any
; means.

; In jurisdictions that recognize copyright laws, the author or authors
; of this software dedicate any and all copyright interest in the
; software to the public domain. We make this dedication for the benefit
; of the public at large and to the detriment of our heirs and
; successors. We intend this dedication to be an overt act of
; relinquishment in perpetuity of all present and future rights to this
; software under copyright law.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
; IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
; OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
; ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
; OTHER DEALINGS IN THE SOFTWARE.
;
;For more information, please refer to <http://unlicense.org/>
;


	IFNDEF	VIEW_SEL
VIEW_SEL	EQU	0
	ENDIF


;--------------------------------------
; MACROs

CODE_BEGIN	MACRO
		.code
ENDM
CODE_END	MACRO
ENDM
DATA_BEGIN	MACRO
		.data
ENDM
DATA_END	MACRO
ENDM
STACK_BEGIN	MACRO
		.stack
ENDM
STACK_END	MACRO
ENDM

MOVOFS		MACRO	r, o
		mov	r, offset FLAT: o
ENDM

;--------------------------------------
;


		.386p
		.MODEL	FLAT

	CODE_BEGIN

startup:
		mov	[initial_esp], esp
		push	eax
		push	edx

		call	startup_02
startup_02:
		pop	eax
		sub	eax, (startup_02 - startup)
		mov	[initial_eip], eax
		MOVOFS	edx, msgBegin
		call	PutS
		mov	ax, cs
		call	PutH16
		mov	al, ':'
		call	PutC
		mov	eax, [initial_eip]
		call	PutH32
		MOVOFS	edx, msgBeginSSSP
		call	PutS
		mov	ax, ss
		call	PutH16
		mov	al, ':'
		call	PutC
		mov	eax, [initial_esp]
		call	PutH32
		call	PutN

		pop	edx
		pop	eax

		call	DispPharVer
		call	DispDosBuff

		cmp	byte ptr [optViewSel], 0
		je	@f
		call	DispDescs
@@:

		mov	eax, 4c00h
		int	21h

	CODE_END
	DATA_BEGIN
initial_eip	dd	0
initial_esp	dd	0
msgBegin	db	'The program begins at CS:EIP=', 0
msgBeginSSSP	db	', SS:ESP=', 0
	DATA_END


	DATA_BEGIN
optViewSel	db	VIEW_SEL
	DATA_END


	CODE_BEGIN

DispPharVer	PROC
		MOVOFS	edx, msgPharVer
		call	PutS
		mov	byte ptr [is_phar], 0
		mov	eax, 3000h
		mov	ebx, 50484152h
		xor	ecx, ecx
		xor	edx, edx
		int	21h
		mov	[phar_id], eax
		mov	[phar_ver], ebx
		push	edx
		MOVOFS	edx, msgEAX1
		call	PutS
		call	PutH32
		ror	eax, 16
		cmp	ax, 4458h
		jne	@f
		mov	byte ptr [is_phar], 1
@@:
		MOVOFS	edx, msgEBX
		call	PutS
		mov	eax, ebx
		call	PutH32
		cmp	byte ptr [is_phar], 0
		jz	@f
		mov	al, '('
		call	PutC
		mov	eax, ebx
		push	ecx
		mov	ecx, 4
@dpv_bx_lp:
		call	PutC
		ror	eax, 8
		loop	short @dpv_bx_lp
		pop	ecx
		mov	al, ')'
		call	PutC
@@:
		MOVOFS	edx, msgECX
		call	PutS
		mov	eax, ecx
		call	PutH32
		cmp	byte ptr [is_phar], 0
		jz	@dpv_dx
		mov	al, '('
		call	PutC
		push	ecx
		mov	eax, ecx
		mov	ecx, 4
@dpv_cx_lp:
		rol	eax, 8
		test	al, al
		jz	@f
		call	PutC
@@:
		loop	@dpv_cx_lp
		pop	ecx
		mov	al, ')'
		call	PutC
@dpv_dx:
		MOVOFS	edx, msgEDX
		call	PutS
		pop	edx
		mov	eax, edx
		call	PutH32
		call	PutN
		ret
DispPharVer	ENDP

	CODE_END
	DATA_BEGIN
msgPharVer	db	'* PharLap DOSX version (int 21h ah=30h, ebx=50484152h)...', 13, 10, 0
phar_id		dd	0
phar_ver	dd	0
is_phar		dd	0
	DATA_END

	CODE_BEGIN
DispDosBuff	PROC
		push	edx
		push	ecx
		push	ebx
		MOVOFS	edx, msgDosBuff
		call	PutS
		push	es
		xor	ebx, ebx
		xor	ecx, ecx
		xor	edx, edx
		mov	es, bx
		mov	eax, 2517h
		int	21h
		mov	[dosbuff_seg], es
		mov	[dosbuff_off], ebx
		mov	[dosbuff_len], edx
		pop	es
		pop	ebx
		jnc	@f
		MOVOFS	edx, msgdosbuff_cf
		call	PutS
		jmp	@ddb_exit
@@:
		call	PutN
		MOVOFS	edx, msgdosbuff_esbx
		call	PutS
		mov	ax, [dosbuff_seg]
		call	PutH16
		mov	al, ':'
		call	PutC
		mov	eax, [dosbuff_off]
		call	PutH32
		MOVOFS	edx, msgdosbuff_ecx
		call	PutS
		mov	eax, ecx
		call	PutH32
		test	eax, eax
		jz	@ddb_edx
		mov	al, '('
		call	PutC
		mov	eax, ecx
		shr	eax, 16
		call	PutH16
		mov	al, ':'
		call	PutC
		mov	eax, ecx
		call	PutH16
		mov	al, ')'
		call	PutC
@ddb_edx:
		MOVOFS	edx, msgdosbuff_edx
		call	PutS
		mov	eax, [dosbuff_len]
		call	PutH32
		call	PutN
@ddb_exit:
		pop	ecx
		pop	edx
		ret
DispDosBuff	ENDP
	CODE_END
	DATA_BEGIN
msgDosBuff	db	'* Get DOS data buffer info (int 21h, ax=2517h)...', 0
msgdosbuff_cf	db	'(CF=1)', 13, 10, 0
msgdosbuff_esbx	db	'ES:EBX=', 0
msgdosbuff_ecx	db	' ECX=', 0
msgdosbuff_edx	db	' EDX=', 0

		ALIGN	4
dosbuff_off	dd	0
dosbuff_seg	dw	0

		ALIGN	4
dosbuff_len	dd	0
	DATA_END

	CODE_BEGIN
DispDescs	PROC
		MOVOFS	edx, msgDispDescs
		call	PutS
		call	DispSegsub
		MOVOFS	edx, msgdispdescs_2
		call	PutS
		MOVOFS	esi, desctbl
@ddsc_lp:
		lodsd
		test	eax, eax
		jz	@ddsc_brk
		call	DispSel
		jmp	short @ddsc_lp
@ddsc_brk:
		ret
DispDescs	ENDP
	CODE_END
	DATA_BEGIN
msgDispDescs	db	'* Descriptors (', 0
msgdispdescs_2	db	')', 13, 10, 0
desctbl:
		dd	4, 8, 0ch
		dd	10h, 14h, 18h, 1ch
		dd	20h, 24h, 28h, 2ch
		dd	30h, 34h, 38h, 3ch
		dd	40h, 44h, 48h, 4ch
		dd	50h
		dd	60h
		dd	0
	DATA_END

	CODE_BEGIN
DispSel		PROC
		push	eax
		push	ebx
		push	ecx
		push	edx
		MOVOFS	edx, msgSel
		call	PutS
		call	PutH16
		MOVOFS	edx, msgSel2
		call	PutS
		push	eax
		mov	ebx, eax
		xor	ecx, ecx
		mov	eax, 2508h
		int	21h
		mov	eax, ecx
		call	PutH32
		MOVOFS	edx, msgSel3
		call	PutS
		pop	eax
		xor	ebx, ebx
		lsl	ebx, eax
		xchg	eax, ebx
		call	PutH32
		call	PutN
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		ret
DispSel		ENDP
	CODE_END
	DATA_BEGIN
msgSel		db	'Selector ', 0
msgSel2		db	' : base=', 0
msgSel3		db	', limit=', 0
	DATA_END

;--------------------------------------
; subs
;

	CODE_BEGIN

PutC		PROC
		push	ebx
		push	ecx
		push	edx
		push	eax
		mov	edx, esp
		push	ds
		push	ss
		pop	ds
		mov	eax, 4000h	; write
		mov	ecx, 1		; 1 byte
		mov	ebx, ecx	; to handle:1 (stdout)
		int	21h
		pop	ds
		pop	eax
		pop	edx
		pop	ecx
		pop	ebx
		ret
PutC		ENDP

PutH4		PROC
		push	eax
		and	al, 0fh
		add	al, '0'
		cmp	al, '9'
		jbe	@f
		add	al, 'A' - '9' - 1
@@:
		call	PutC
		pop	eax
		ret
PutH4		ENDP

PutH8		PROC
		push	eax
		shr	al, 4
		call	PutH4
		pop	eax
		jmp	short PutH4
PutH8		ENDP

PutH16		PROC
		xchg	al, ah
		call	PutH8
		xchg	al, ah
		jmp	short PutH8
PutH16		ENDP

PutH32		PROC
		rol	eax, 16
		call	PutH16
		rol	eax, 16
		jmp	short PutH16
PutH32		ENDP


PutS		PROC
		push	eax
		push	esi
		mov	esi, edx
@puts_lp:
		mov	al, [esi]	; do not touch DF...
		test	al, al
		jz	@f
		call	PutC
		inc	esi
		jmp	short @puts_lp
@@:
		pop	esi
		pop	eax
		ret
PutS		ENDP

PutN		PROC
		push	eax
		mov	al, 13
		call	PutC
		mov	al, 10
		call	PutC
		pop	eax
		ret
PutN		ENDP



DispSR32	PROC
		call	PutS
		call	PutH32
		ret
DispSR32	ENDP

DispSR16	PROC
		call	PutS
		call	PutH16
		ret
DispSR16	ENDP

DispABCDXsub	PROC
		push	edx
		MOVOFS	edx, msgEAX1
		call	DispSR32
		MOVOFS	edx, msgEBX
		mov	eax, ebx
		call	DispSR32
		MOVOFS	edx, msgECX
		mov	eax, ecx
		call	DispSR32
		pop	edx
		mov	eax, edx
		MOVOFS	edx, msgEDX
		call	DispSR32
		ret
DispABCDXsub	ENDP

DispSegsub	PROC
		MOVOFS	edx, msgCS1
		mov	ax, cs
		call	DispSR16
		MOVOFS	edx, msgDS
		mov	ax, ds
		call	DispSR16
		MOVOFS	edx, msgES
		mov	ax, es
		call	DispSR16
		MOVOFS	edx, msgFS
		mov	ax, fs
		call	DispSR16
		MOVOFS	edx, msgGS
		mov	ax, gs
		call	DispSR16
		MOVOFS	edx, msgSS
		mov	ax, ss
		call	DispSR16
		ret
DispSegsub	ENDP


DispRegsSegs	PROC
		push	eax
		push	edx
		call	DispABCDXsub
		call	PutN
		MOVOFS	edx, msgESI1
		call	PutS
		mov	eax, esi
		call	PutH32
		MOVOFS	edx, msgEDI
		call	PutS
		mov	eax, edi
		call	PutH32
		MOVOFS	edx, msgEBP
		call	PutS
		mov	eax, ebp
		call	PutH32
		MOVOFS	edx, msgESP
		call	PutS
		mov	eax, [initial_esp]
		call	PutH32
		call	PutN
		call	DispSegsub
		call	PutN
		pop	edx
		pop	eax
		ret
DispRegsSegs	ENDP

	DATA_BEGIN
msgEAX		db	' '
msgEAX1		db	'EAX=', 0
msgEBX		db	' EBX=', 0
msgECX		db	' ECX=', 0
msgEDX		db	' EDX=', 0
msgEFLAGS	db	' EFLAGS=', 0
msgESI		db	' '
msgESI1		db	'ESI=', 0
msgEDI		db	' EDI=', 0
msgEBP		db	' EBP=', 0
msgESP		db	' ESP=', 0
msgCS		db	' '
msgCS1		db	'CS=', 0
msgDS		db	' DS=', 0
msgES		db	' ES=', 0
msgFS		db	' FS=', 0
msgGS		db	' GS=', 0
msgSS		db	' SS=', 0
	DATA_END


	STACK_BEGIN
		dd	1024 dup (?)
	STACK_END

		END	startup

