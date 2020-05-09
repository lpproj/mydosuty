; this file is a part of test1b06


	.model small
	.dosseg
	.code

CONVTYPE	TEXTEQU	C

EXPSYM MACRO sym
	public CONVTYPE sym
ENDM

	EXPSYM	intxx_value

CallIntXX	PROC CONVTYPE PUBLIC
	pushf
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es

	push	word ptr cs: [r_es0]
	push	word ptr cs: [r_ds0]
	push	word ptr cs: [r_bp0]
	push	word ptr cs: [r_di0]
	push	word ptr cs: [r_si0]
	push	word ptr cs: [r_dx0]
	push	word ptr cs: [r_cx0]
	push	word ptr cs: [r_bx0]
	push	word ptr cs: [r_ax0]
	push	word ptr cs: [r_flags0]
	popf
	pop	ax
	pop	bx
	pop	cx
	pop	dx
	pop	si
	pop	di
	pop	bp
	pop	ds
	pop	es
	mov	word ptr cs: [org_ss], ss
	mov	word ptr cs: [org_sp], sp
	push	cs
	pop	ss
	mov	sp, offset private_stack_bottom

		db	0cdh
intxx_value	db	1bh

	mov	ss, word ptr cs: [org_ss]
	mov	sp, word ptr cs: [org_sp]

	pushf
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es
	pop	word ptr cs: [r_es1]
	pop	word ptr cs: [r_ds1]
	pop	word ptr cs: [r_bp1]
	pop	word ptr cs: [r_di1]
	pop	word ptr cs: [r_si1]
	pop	word ptr cs: [r_dx1]
	pop	word ptr cs: [r_cx1]
	pop	word ptr cs: [r_bx1]
	pop	word ptr cs: [r_ax1]
	pop	word ptr cs: [r_flags1]

	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	popf
	ret
CallIntXX	ENDP

	even

	EXPSYM r_ax0
	EXPSYM r_bx0
	EXPSYM r_cx0
	EXPSYM r_dx0
	EXPSYM r_si0
	EXPSYM r_di0
	EXPSYM r_bp0
	EXPSYM r_flags0
	EXPSYM r_ds0
	EXPSYM r_es0
	EXPSYM r_ax1
	EXPSYM r_bx1
	EXPSYM r_cx1
	EXPSYM r_dx1
	EXPSYM r_si1
	EXPSYM r_di1
	EXPSYM r_bp1
	EXPSYM r_flags1
	EXPSYM r_ds1
	EXPSYM r_es1

	EXPSYM r_pack0
	EXPSYM r_pack1

	EXPSYM private_stack
	EXPSYM private_stack_bottom

r_pack0:
r_ax0		dw	0
r_bx0		dw	0
r_cx0		dw	0
r_dx0		dw	0
r_si0		dw	0
r_di0		dw	0
r_bp0		dw	0
r_flags0	dw	0
r_ds0		dw	0
r_es0		dw	0
r_pack1:
r_ax1		dw	0
r_bx1		dw	0
r_cx1		dw	0
r_dx1		dw	0
r_si1		dw	0
r_di1		dw	0
r_bp1		dw	0
r_flags1	dw	0
r_ds1		dw	0
r_es1		dw	0

org_sp		dw	0
org_ss		dw	0

private_stack:
		dw	256 dup (0)
private_stack_bottom:
		dw	0deadh


	end

