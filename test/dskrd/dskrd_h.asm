%if 0	; %comment

	dskrd_h.asm: a part of dskrd

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


	CPU 8086
	BITS 16

%ifdef LSI_C
  %ifdef FARCODE
segment dskrd_TEXT align=1 class=CODE
  %else
group CGROUP TEXT
segment TEXT align=1 class=CODE
  %endif
%define rr32l ax
%define rr32h bx
%else
segment _TEXT align=2 class=CODE
%define rr32l ax
%define rr32h dx
%endif

%ifdef FARCODE
  %define CDECL_PARAM_OFFSET 6
  %define RET_PROC retf
%else
  %define CDECL_PARAM_OFFSET 4
  %define RET_PROC ret
%endif


	global	_my_get_flags
	global	my_get_flags_
_my_get_flags:
my_get_flags_:
	pushf
	pop	ax
	RET_PROC

	global	_my_get_regpack_call
	global	my_get_regpack_call_
_my_get_regpack_call:
my_get_regpack_call_:
	mov	rr32l, my_regpack_call
	push	cs
	pop	rr32h
	RET_PROC

	global	_my_get_regpack_ret
	global	my_get_regpack_ret_
_my_get_regpack_ret:
my_get_regpack_ret_:
	mov	rr32l, my_regpack_ret
	push	cs
	pop	rr32h
	RET_PROC

	global	_my_get_intcall_invocation
	global	my_get_intcall_invocation_
_my_get_intcall_invocation:
my_get_intcall_invocation_:
	mov	rr32l, my_intcall_actual_invocation
	push	cs
	pop	rr32h
	RET_PROC


	global	_my_intcall_body
	global	my_intcall_body_

_my_intcall_body:
my_intcall_body_:
	push	bp
;	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	es
	pushf
;
	push	cs
	pop	ds
	mov	[my_prev_sp], sp
	mov	[my_prev_ss], ss
;
	mov	ax, [my_ax_c]
	mov	bx, [my_bx_c]
	mov	cx, [my_cx_c]
	mov	dx, [my_dx_c]
	mov	si, [my_si_c]
	mov	di, [my_di_c]
	mov	bp, [my_bp_c]
	mov	es, [my_es_c]
	push	word [my_flags_c]
	mov	ds, [my_ds_c]
	popf
	pushf				; push flags before calling, to be safe...
	mov	[cs: my_sp_c], sp
	mov	[cs: my_ss_c], ss
	;
my_intcall_actual_invocation:
	db	0cdh, 21h		; int xxh
	db	90h, 90h, 90h, 90h	; stub for far-call
	;
	mov	[cs: my_sp_r], sp
	mov	[cs: my_ss_r], ss
	;cli
	mov	ss, [cs: my_prev_ss]
	mov	sp, [cs: my_prev_sp]
	mov	[cs: my_ds_r], ds
	push	cs
	pop	ds
	pushf
	pop	word [my_flags_r]
	mov	[my_ax_r], ax
	mov	[my_bx_r], bx
	mov	[my_cx_r], cx
	mov	[my_dx_r], dx
	mov	[my_si_r], si
	mov	[my_di_r], di
	mov	[my_bp_r], bp
	mov	[my_es_r], es
	;
	sbb	ax, ax		; ax = -1 if CF=1
	popf
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
;	pop	ax
	pop	bp
	RET_PROC

my_prev_sp	dw	0
my_prev_ss	dw	0

my_regpack_call:
my_flags_c	dw	0
my_es_c		dw	0
my_ds_c		dw	0
my_di_c		dw	0
my_si_c		dw	0
my_bp_c		dw	0
my_sp_c		dw	0
my_bx_c		dw	0
my_cx_c		dw	0
my_dx_c		dw	0
my_ax_c		dw	0
my_ss_c		dw	0

my_regpack_ret:
my_flags_r	dw	0
my_es_r		dw	0
my_ds_r		dw	0
my_di_r		dw	0
my_si_r		dw	0
my_bp_r		dw	0
my_sp_r		dw	0
my_bx_r		dw	0
my_cx_r		dw	0
my_dx_r		dw	0
my_ax_r		dw	0
my_ss_r		dw	0


