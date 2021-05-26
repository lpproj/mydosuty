; pc98thdl.asm
; this file is a part of pc98tick

; definition of segment order
%ifdef LSI_C
GROUP CGROUP TEXT
GROUP DGROUP DATA BSS
segment TEXT align=1 class=CODE
segment DATA align=2 class=DATA
segment BSS align=2 class=DATA
%define CODESEG		segment TEXT
%macro CSYMDEF 1
	global %{1}_
%{1}_:
%{1}:
%endmacro
%else
segment _TEXT align=2 class=CODE
%define CODESEG		segment _TEXT
%macro CSYMDEF 1
	global _%{1}
_%{1}:
%{1}:
%endmacro
%endif

%macro IOWAIT 0
	out	5fh, al
%endmacro


CODESEG

	CSYMDEF	org_int08
	dd	0
	CSYMDEF	timer_ticks
	dd	0
	CSYMDEF	pit_rearm
	dw	0
	CSYMDEF	do_chain
	db	1

	align	2
;
	CSYMDEF	new_int08
	push	ax
	push	ds
	xor	ax, ax
	mov	ds, ax
	add	word [cs: timer_ticks], 1
	adc	word [cs: timer_ticks + 2], ax		; ax=0
	dec	ax					; ax=ffffh
	mov	word [058ah], ax
	pop	ds
	test	byte [cs: do_chain], al
	jz	rearm_pit_and_iret
	pop	ax
	jmp	far [cs: org_int08]

rearm_pit_and_iret:
	mov	al, 20h
	out	00h, al		; EOI
	IOWAIT
	mov	al, 36h
	out	77h, al
	IOWAIT
	mov	ax, [cs: pit_rearm]
	out	71h, al
	xchg	al, ah
	IOWAIT
	out	71h, al
	pop	ax
	iret

