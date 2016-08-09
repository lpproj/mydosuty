; bootdumy.asm
; a minimal chain-boot loader for IBMPC and NEC PC-9801/9821
; (from FD to 1st HD)
%if 0
  Copyright (C) 2015-2016 sava (t.ebisawa)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
%endif
; (in short term: `under the ZLIB license') 

    ORG 0
    USE16
    CPU 8086

                                ; bootstrap loader will be loaded at:
offset_ibmpc      equ 7c00h     ; IBMPC   0000:7c00
offset_bt2_ibmpc  equ 0600h
offset_nec98      equ 0         ; PC-98x1 1fe0:0000 (1fc0:0000 for FD)
segment_bt2_nec98 equ 0060h
DISK_RETRY        equ 3
NEC98_BOOT_DRIVE  equ 0584h
NEC98_DISK_EQUIP  equ 055dh


boot0:
    jmp short boot1
    nop
    db  'BOOTSTUB'
    
    ;resb 0x3e - ($ - boot0)
    times (0x26 - ($ - boot0)) db 0
    db 29h            ; extended boot record
    dd 0deadbeefh     ; volume serial number
    db 'NO NAME    '  ; disk label
    db 'FAT     '     ; file system
boot1:
    cld
    mov cx, msgNoSystem_end - msgNoSystem
    ; detect machine which is PC-98x1/IBM by int 1Ah
    ; (need more bytes than by int 10h ah=0Fh, but probably safer...)
    mov dh, 0ffh
    mov ah, 0
    int 1ah
    inc dh
    jnz boot1_ibmpc
    mov dh, 0feh
    int 1Ah
    cmp dh, 0feh
    je boot1_nec98

boot1_ibmpc:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov si, msgNoSystem + offset_ibmpc
.l1_ibm:
    lodsb
    mov ah, 0eh
    int 10h
    loop .l1_ibm
.l2_ibm:
    mov ah, 0
    int 16h
    mov ax, 0e0dh
    int 10h
    mov ax, 0e0ah
    int 10h
    ; copy bootreal code to outside ...
    mov si, bootreal_ibmpc + offset_ibmpc
    mov di, offset_bt2_ibmpc
    mov cx, bootreal_ibmpc_end - bootreal_ibmpc
    rep movsb
    db 0eah
    dw offset_bt2_ibmpc, 0  ; jmp 0000:offset_bt2_ibmpc
    
;
boot1_nec98:
    push cs
    pop ds
    mov ah, 0ch         ; show screen
    int 18h
    mov ax, 0a000h
    mov es, ax
    xor di, di
    mov si, msgNoSystem + offset_nec98
    xor ah, ah
.lp_nec:
    lodsb
    stosw
    loop .lp_nec
    mov ah, 11h         ; show cursor
    int 18h
    mov ah, 13h         ; set cursor addr.
    mov dx, di
    int 18h
    xor ax, ax          ; get keycode (with wait)
    int 18h
    mov ah, 12h
    int 18h             ; hide cursor
    mov ah, 13h         ; move cursor to home
    xor dx, dx
    int 18h
    mov si, bootreal_nec98 + offset_nec98
    mov ax, segment_bt2_nec98
    mov es, ax
    xor di, di
    mov cx, bootreal_nec98_end - bootreal_nec98
    rep movsb
    db 0eah
    dw 0, segment_bt2_nec98     ; jmp segment_bt2_nec98:0000


;

bootreal_ibmpc:
    mov ax, 0
    mov dl, 80h
    int 13h                     ; wake up, HDD
    mov cx, DISK_RETRY
.bt_ibm_lp:
    push cx
    xor ax, ax
    mov dl, 80h
    int 13h
    mov ax, 0201h
    mov bx, 7c00h
    mov dx, 0080h
    mov cx, 01h
    int 13h
    pop cx
    jnc .bt_ibm_go
    loop .bt_ibm_lp
.fall_ibmpc:
    int 19h                     ; INT 19 - SYSTEM - BOOTSTRAP LOADER (RBINT)
.bt_ibm_go:
    jmp 0000:7c00h

bootreal_ibmpc_end:


bootreal_nec98:
    ;push cs
    ;pop ds
    xor ax, ax
    mov ds, ax
    mov ah, [NEC98_DISK_EQUIP]  ; DISK_EQUIP
    mov al, 80h
    test ah, 1
    jnz .bt_nec_ide
    inc al
    test ah, 2
    jz .fall_nec98
.bt_nec_ide:
    mov [NEC98_BOOT_DRIVE], al
    mov ah, 8eh                 ; hdd "half-height" mode
    int 1bh           ; (undoc1:int1b_h.txt, undoc2:memsys.txt, disktut.txt)
                      ; needed for real machines (with an IDE drive)...
    mov ax, 1fe0h
    mov es, ax        ; load at 1fe0:0000 (es:bp)
    xor bp, bp
    push ax           ; prepare to go (by retf)
    push bp
    mov cx, DISK_RETRY
.bt_nec_lp:
    push cx
    mov al, [NEC98_BOOT_DRIVE]
    mov ah, 7
    int 1bh
    mov ah, 06h
    xor cx, cx
    xor dx, dx
    mov bx, 512
    int 1bh
    pop cx
    jnc .bt_nec_go
    loop .bt_nec_lp
.fall_nec98:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov dx, [0486h]   ; for a proof: restore DX (initial value after reset)
    jmp 0f000h:0fff0h ; 0ffffh:0000h for old 8086/186
.bt_nec_go:
    retf                ; jmp to the top (1fe0:0000)
bootreal_nec98_end:

msgNoSystem db 'No System Disk. Press any key to boot up from HD...'
msgNoSystem_end:

