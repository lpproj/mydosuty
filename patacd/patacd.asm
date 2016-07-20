%if 0
;------------------------------------------------------------------------------
patacd.asm - IDE (Parallel ATA) CD-ROM driver for MS-DOS

target:  MS-DOS 3.1 or later
         IBM PC/AT compatible or NEC PC-9801 compatible
author:  lpproj
license: the ZLIB license

  Copyright (C) 2016 sava (t.ebisawa)

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


to build:
    nasm -f bin -o patacd.sys patacd.asm

;------------------------------------------------------------------------------
%endif

;%define PRIVATE_STACK_SIZE 96

%ifndef PRIVATE_STACK_SIZE
%define PRIVATE_STACK_SIZE 0
%endif

%define SUPPORT_CD_PLAY
;%define SUPPORT_AUDIO_CHANNEL
;%define SUPPORT_DRIVE_BYTES
;%define SUPPORT_RW_SUBCHANNELS


%ifndef SUPPORT_CD_PLAY
%undef SUPPORT_AUDIO_CHANNEL
%endif


CR     equ 13
LF     equ 10
eos    equ '$'

    CPU 286
    BITS 16


IDX_DATA                equ 0
IDX_FEATURES            equ 1
IDX_ERROR               equ IDX_FEATURES
IDX_SECTOR_COUNT        equ 2
IDX_INTERRUPT_REASON    equ IDX_SECTOR_COUNT
IDX_SECTOR_NUMBER       equ 3
IDX_CYLINDER_L          equ 4
IDX_BYTECOUNT_L         equ IDX_CYLINDER_L
IDX_CYLINDER_H          equ 5
IDX_BYTECOUNT_H         equ IDX_CYLINDER_H
IDX_DEVICE              equ 6
IDX_COMMAND             equ 7
IDX_STATUS              equ IDX_COMMAND
IDX_DEVICE_CONTROL      equ 8
IDX_ALTERNATE_STATUS    equ IDX_DEVICE_CONTROL

OFS_DATA                equ (IDX_DATA * 2)
OFS_FEATURES            equ (IDX_FEATURES * 2)
OFS_ERROR               equ (IDX_ERROR * 2)
OFS_SECTOR_COUNT        equ (IDX_SECTOR_COUNT * 2)
OFS_INTERRUPT_REASON    equ (IDX_INTERRUPT_REASON * 2)
OFS_SECTOR_NUMBER       equ (IDX_SECTOR_NUMBER * 2)
OFS_CYLINDER_L          equ (IDX_CYLINDER_L * 2)
OFS_CYLINDER_H          equ (IDX_CYLINDER_H * 2)
OFS_BYTECOUNT_L         equ (IDX_BYTECOUNT_L * 2)
OFS_BYTECOUNT_H         equ (IDX_BYTECOUNT_H * 2)
OFS_DEVICE              equ (IDX_DEVICE * 2)
OFS_COMMAND             equ (IDX_COMMAND * 2)
OFS_STATUS              equ (IDX_STATUS * 2)
OFS_DEVICE_CONTROL      equ (IDX_DEVICE_CONTROL * 2)
OFS_ALTERNATE_STATUS    equ (IDX_ALTERNATE_STATUS * 2)

IBM_TIMER_TICKS         equ 046ch       ; 0040:006C dword
NEC98_TIMEOUT_CHECKCNT  equ 250
NEC98_ATA_BANKSEL       equ 432h
NEC98_PORT_WAIT600ns    equ 5fh

ATA_TIMEOUT_DEVICE_READY  equ 2
ATA_TIMEOUT_DEVICE_SELECT equ 2
ATA_TIMEOUT_COMMAND_SEND  equ 2
ATA_TIMEOUT_IDENTIFY      equ 2
ATA_TIMEOUT_NORMAL        equ 5
ATA_TIMEOUT_DISC          equ 30


struc TVALnec98
    .checkcnt     resw 1
    .prev_sec:    resb 1
    .reserved:    resb 1
    .remain_sec:  resw 1
endstruc

struc TVALibm
    .reserved:    resw 1
    .prev_tick:   resw 1
    .remain_55ms: resw 1
endstruc


%ifidn __OUTPUT_FORMAT__, bin
    SECTION .text
%else
; for embedded in other apps
segment DEVICE_TEXT align=16 class=CODE
    group DEVGROUP       DEVICE_TEXT
%endif

device_header:

    dw 0ffffh, 0ffffh
device_attributes:
    dw 0c800h                 ; CharDev, IOCTL, Open/Close
    dw Strategy
device_commands_entry:
%ifdef TEST
    dw Commands
%else
    dw Init_Commands
%endif
device_name:
    db '$CDROM$$'               ; dummy name
                                ; 'CD_101  ' NEC98 default
                                ; 'MSCD001 ' PCAT default
    dw 0                        ; cddriver: reserved
    db 0                        ; cddriver: drive letter (1=A, 2=B,...)
    db 1                        ; cddriver: number of units


; cd-rom drive

cd_drvnum:
    db 02h		; 0ffh
cd_media_inserted:
    db 00h
cd_media_changed:
    db 01h
cd_need_update:
    db 01h
cd_device_status:
    dw 00h

cd_buf:
    times 64 db 0

%ifdef SUPPORT_CD_PLAY
cd_toc_buf_header:
    dw 0
cd_toc_first_track:
    db 0
cd_toc_last_track:
    db 0
ata_identify_buf:
cd_toc_buf:
    times 4 * 100 db 0
;    times 4 * 100 db 0
cd_toc_pleadout_msf:
    dw 0
cd_toc_leadout_lba:
    dd 0
%endif
cd_capacity:
    dd 0

tval:
    times 6 db 0      ; max(TVALnec98_size, TVALibm_size)

ata_wait_bsyvalid:
    dw Wait400ns_ibm
ata_select_ports:
    dw ATASelectPorts_ibm
set_timeout_count:
    dw SetTimeoutCount_ibm
is_timeout:
    dw isTimeout_ibm

ata_drivenum:
    dw 0002h		; (0..3)
ata_portbase:
    dw 0

atareg:
    db 0            ; (reserved)
    db 0            ; Features
    db 0            ; Sector Count
    db 0            ; Sector Number
    db 0            ; Cylinder/Byte Count Low
    db 0            ; Cylinder/Byte Count High
    db 10100000b    ; Device/Head  obs=1
    db 0            ; Command
    db 00001010b    ; Device Control  nIEN=1 bit3=1


ata_lasterror:
    db 0
atapi_interrupt_reason:
    db 0
atapi_asc:
    db 0
atapi_ascq:
    db 0

is_nec98:
    db 0
isNECCD260:
    db 0

atapi_read_data:
    dw ATAReadData
atapi_write_data:
    dw ATAWriteData

atapi_packet_size:
    dw 12           ; 12 or 16
atapi_packet:
    times 16 db 0
atapi_max_bytecount:
    dw 0
atapi_bytecount:
    dw 0


wait_bsy0_atapi:
    mov dx, word [bx + OFS_ALTERNATE_STATUS]
    in al, dx
    mov dx, [bx + OFS_STATUS]
    jmp short wait_bsy0.lp

wait_bsy0:
    mov dx, word [bx + OFS_ALTERNATE_STATUS]
  .lp:
    in al, dx
    test al, 80h
    jnz .wait
    ret
  .wait:
    sti
    call [is_timeout]
    jnc .lp
    mov ax, 0ffffh
    ret

wait_drq0:
    mov dx, word [bx + OFS_ALTERNATE_STATUS]
  .lp:
    in al, dx
    test al, 88h
    jnz .wait
    ret
  .wait:
    sti
    call [is_timeout]
    jnc .lp
    mov ax, 0ffffh
    ret
    

wait_drq1:
    mov dx, word [bx + OFS_ALTERNATE_STATUS]
  .lp:
    in al, dx
    test al, 80h  ; check BSY = 0
    jnz .wait
    test al, 08h  ; check DRQ = 1
    jz .wait
    ret
  .wait:
    sti
    call [is_timeout]
    jnc .lp
    mov ax, 0ffffh
    ret

wait_drdy1:
    mov dx, word [bx + OFS_ALTERNATE_STATUS]
  .lp:
    in al, dx
    test al, 80h  ; check BSY = 0
    jnz .wait
    test al, 40h  ; check DRDY = 1
    jz .wait
    ret           ; CF=0 normal exit
  .wait:
    sti
    call [is_timeout]
    jnc .lp
    mov ax, 0ffffh
    ret



;
; ATAPrepareSelectDevice
; select port and prepare device register DEV bit
;
; in
; [ata_drivenum]    drive (0..3)
; ds      cs
;
; out
; [atareg.Device]   set or clear DEV bit
; bx                ata portbase
; [ata_portbase]    ditto
; CF                0 no error
;                   1 error (al > 3)
;


ATAPrepareSelectDevice:
    push ax
    mov ax, [ata_drivenum]
    call word [ata_select_ports]
    mov [ata_portbase], bx
    mov ah, 3
    sub ah, al
    jc .exit
    and al, 1
    shl al, 4
    mov ah, byte [atareg + IDX_DEVICE]
    and ah, 0efh
    or al, ah
    mov byte [atareg + IDX_DEVICE], al
  .exit:
    pop ax
    ret
    


; ATASelectDevice/ATAWriteDeviceRegister
;
; in
; [atareg]        parameter to ATA(PI) drive
; bx              ata_portbase
; ds              same as cs
;
; out
; AX              result
;                 0..ff result register
;                 -1 timeout
;                 0, 1 DEV bit
; CF              1 error
;
; DX              modified
;

ATASelectDevice_withTimeout:
    push ax
    mov ax, ATA_TIMEOUT_DEVICE_SELECT
    call [set_timeout_count]
    pop ax
ATASelectDevice:
  .lp1:
    mov al, [atareg + IDX_DEVICE]
    mov dx, word [bx + OFS_DEVICE]
    out dx, al
    call [ata_wait_bsyvalid]  ; (need 400ns wait)
    mov dx, word [bx + OFS_STATUS]
    in al, dx
    test al, 88h  ; check BSY=0 and DRQ=0
    jnz .lp_check_timeout
    mov dx, [bx + OFS_DEVICE]		; check DEV bit
    in al, dx
    mov ah, [atareg + IDX_DEVICE]
    and ax, 1010h
    cmp ah, al
    je .noerr
    mov ah, 0ffh
    jmp short .error
  .noerr:
    xor ah, ah
    ret
  .lp_check_timeout:
    call [ata_wait_bsyvalid]  ; (wait for a proof...)
    call [is_timeout]
    jnc .lp1
  .timeout:
    mov ax, 0ffffh
  .error:
    stc
    ret

; ATASendCommand
; in
; BX        ataports base
; atareg    registers
;
; out
; CY        1 = err
; AX        -1 = timeout
; AH        1 = err
; AL        result status
;
ATASendCommand:
    call ATASelectDevice    ; select device and write Drive/Head reg
    mov dx, [bx + OFS_DEVICE_CONTROL]
    mov al, [atareg + IDX_DEVICE_CONTROL]
;    and al, 0fbh          ; SRST = 0 (no reset)
;    or al, 02h            ; nIEN = 1 (disable int)
    out dx, al
    mov dx, [bx + OFS_FEATURES]
    mov al, [atareg + IDX_FEATURES]
    out dx, al
    mov dx, [bx + OFS_SECTOR_COUNT]
    mov al, [atareg + IDX_SECTOR_COUNT]
    out dx, al
    mov dx, [bx + OFS_SECTOR_NUMBER]
    mov al, [atareg + IDX_SECTOR_NUMBER]
    out dx, al
    mov dx, [bx + OFS_CYLINDER_L]
    mov al, [atareg + IDX_CYLINDER_L]
    out dx, al
    mov dx, [bx + OFS_CYLINDER_H]
    mov al, [atareg + IDX_CYLINDER_H]
    out dx, al
    ; write command
    mov dx, [bx + OFS_COMMAND]
    mov al, [atareg + IDX_COMMAND]
    out dx, al
    call [ata_wait_bsyvalid]
    mov dx, [bx + OFS_ALTERNATE_STATUS] ; read Alternate Status Register
    in al, dx                           ; and drop it
    call wait_bsy0
    jc ata_checkstatus.exit          ; timeout
    ; fallthrough
ata_checkstatus:
    mov dx, [bx + OFS_STATUS]
    in al, dx
    mov ah, al
    and ah, 1
    jnz ata_setlasterror
  .exit:
    ret
ata_setlasterror:
    push ax
    push dx
    mov dx, [bx + OFS_ERROR]
    in al, dx
    mov byte [ata_lasterror], al
    pop dx
    pop ax
    stc
    ret

ATAReadStatus:
    mov dx, [bx + OFS_ALTERNATE_STATUS]
  .l2:
    in al, dx
    call [ata_wait_bsyvalid]  ; (just for a proof)
    jmp short ata_checkstatus

ATAPIReadStatus:
    mov dx, [bx + OFS_ALTERNATE_STATUS]
    in al, dx
    mov dx, [bx + OFS_STATUS]
    jmp short ATAReadStatus.l2


; ATAReadData
;
; in
; es:si  pointer to data read
; cx     read bytes
; bx     ata_portbase
; ds     same as cs
; df = 0 (cld)
;
ATAReadData:
    mov dx, [bx]    ; bx + OFS_DATA
    push cx
    push di
    mov di, si
    add cx, 1       ; (cx+1)/2
    rcr cx, 1
    rep insw
    pop di
    pop cx
    ret


; ATAWriteData
;
; in
; es:si  pointer to data write
; cx     write bytes
; bx     ata_portbase
; ds     same as cs
; df = 0 (cld)
;
ATAWriteData:
    mov dx, [bx]    ; bx + OFS_DATA
    push cx
    push si
    push ds
    push es
    pop ds
    add cx, 1       ; (cx+1)/2
    rcr cx, 1
    rep outsw
    pop ds
    pop si
    pop cx
    ret

;
; ATAPostProcess
; re-enable INTRQ on the port, for HD BIOS
;
ATAPostProcess:
    pushf
    push ax
    mov dx, [bx + OFS_DEVICE_CONTROL]
    mov al, [atareg + IDX_DEVICE_CONTROL]
    and al, 11111001b      ; SRST=0 (no reset), nIEN=0 (enable INTRQ)
    out dx, al
    pop ax
    popf
    ret


%if 0
ATAPIPostProcess:
    pushf
    push ax
    ; send NOP command
    xor ax, ax
    mov dx, [bx + OFS_FEATURES]
    out dx, al
    mov dx, [bx + OFS_COMMAND]
    out dx, al
    call [ata_wait_bsyvalid]  ; (need 400ns wait)
    mov dx, word [bx + OFS_STATUS]
  .lp:
    in al, dx
    test al, 80h  ; check BSY=0 and DRQ=0
    jz .l2
    jmp short .lp
  .l2:
    mov dx, [BX + OFS_STATUS]
    in al, dx
    mov dx, [bx + OFS_DEVICE_CONTROL]
    mov al, [atareg + IDX_DEVICE_CONTROL]
    and al, 11111001b      ; SRST=0 (no reset), nIEN=0 (enable INTRQ)
    out dx, al
    pop ax
    popf
    ret
%endif


;--------------------------------------
; Machine specific part
; IBM   IBM PC/AT, PS/2 or comaptibles
; NEC98 NEC PC-9801/9821 or compatibles



;--------------------------------------
; IBM ATA stuff

ata_ports_ibm_primary:
    dw 1f0h, 1f1h, 1f2h, 1f3h, 1f4h, 1f5h, 1f6h, 1f7h, 3f6h
ata_ports_ibm_secondary:
    dw 170h, 171h, 172h, 173h, 174h, 175h, 176h, 177h, 376h


getlowertick_ibm:
    push ds
    xor ax, ax
    mov ds, ax
    mov ax, word [IBM_TIMER_TICKS]
    pop ds
    ret

SetTimeoutCount_ibm:
    push dx
    push cx
    cmp ax, 3604    ; (55 * 65536 / 1000)
    jb .l2
    mov ax, 0ffffh
    jmp short .store
  .l2:
    mov cx, 1000
    mul cx
    mov cx, 55
    div cx
  .store:
    mov [cs: tval + TVALibm.remain_55ms], ax
    call getlowertick_ibm
    mov [cs: tval + TVALibm.prev_tick], ax
    pop cx
    pop dx
    ret

isTimeout_ibm:
    push ax
    push bx
    push cx
    mov cx, [cs: tval + TVALibm.remain_55ms]
    jcxz .exit_cx
    call getlowertick_ibm
    mov bx, [cs: tval + TVALibm.prev_tick]
    cmp ax, bx
    je .exit          ; return if tick not inc... (with CF=0)
    mov [cs: tval + TVALibm.prev_tick], ax
    ja .diff_normal
    neg bx            ; roundup (e.g. prev FFFFh -> new 0001h)
    add ax, bx
    jmp short .cntdown
  .diff_normal:
    sub ax, bx
  .cntdown:
    sub cx, ax
    jae .cntdown2
    xor cx, cx
  .cntdown2:
    mov [cs: tval + TVALibm.remain_55ms], cx
  .exit_cx:
    cmp cx, 1         ; set CF if cx == 0
  .exit:
    pop cx
    pop bx
    pop ax
    ret


ATASelectPorts_ibm:
    mov bx, ata_ports_ibm_primary
    test al, 2
    jz .l2
    mov bx, ata_ports_ibm_secondary
  .l2:
    ret


Wait400ns_ibm:
    push ax
    mov ah, 3       ; how many does it need...?
  .lp:
    in al, 61h      ; KB controller B (NMISC in intel chipset ref.)
    dec ah
    jne .lp
    pop ax
    ret


;--------------------------------------
; NEC PC-98 IDE stuff

    align 2

ata_ports_nec98:
    dw 640h, 642h, 644h, 646h, 648h, 64ah, 64ch, 64eh, 74ch


getbcdsecond_nec98:
    push bx
    push es
    push ss
    pop es
    sub sp, 6
    mov bx, sp
    push ax
    mov ah, 0
    int 1ch
    pop ax
    add sp, 6
    mov al, byte [es: bx + 5]
    pop es
    pop bx
    ret

SetTimeoutCount_nec98:
    mov [cs: tval + TVALnec98.remain_sec], ax
    call getbcdsecond_nec98
    mov [cs: tval + TVALnec98.prev_sec], al
    mov word [cs: tval], NEC98_TIMEOUT_CHECKCNT
    ret

isTimeout_nec98:
    push ax
    push cx
    mov cx, [cs: tval + TVALnec98.remain_sec]
    or cx, cx     ; check cx == 0 (and clear CF)
    jz .exit_cx
    dec word [cs: tval]   ; (do not modify CF: still 0)
    jnz .exit
    mov word [cs: tval], NEC98_TIMEOUT_CHECKCNT
    call getbcdsecond_nec98
    cmp al, [cs: tval + TVALnec98.prev_sec]
    je .exit
    mov [cs: tval + TVALnec98.prev_sec], al
    dec cx
    mov [cs: tval + TVALnec98.remain_sec], cx
  .exit_cx:
    cmp cx, 1         ; set CF if cx == 0
  .exit:
    pop cx
    pop ax
    ret

ATASelectPorts_nec98:
    push ax
    push dx
    mov dx, NEC98_ATA_BANKSEL
    and al, 2
    shr al, 1
    out dx, al
    out NEC98_PORT_WAIT600ns, al  ; wait for a proof...
    pop dx
    pop ax
    mov bx, ata_ports_nec98
    ret

Wait600ns_nec98:
    out NEC98_PORT_WAIT600ns, al
    ret






; ATAPIOin
;
; in
; ds            cs
; atareg        command and parameters
; bx            ata portbase
; cx            data read count
; es:si         data buffer
; (calling ATASelectDevice is needed before invoke ATAPIOin)
ATAPIOin_withTimeout:
    push ax
    mov ax, ATA_TIMEOUT_NORMAL
    call [set_timeout_count]
    pop ax
ATAPIOin:
    call ATASelectDevice
    jc atapioin_exit.justret
    call wait_drdy1
    jnc ATAPIOin_nodrdy.after_select
    jmp short atapioin_exit

; ATAPIOin_nodrdy
; not wait DRDY=1 on command invokation:
; EXCUTE DEVICE DIAGNOSTIC
; INITIALIZE DEVICE PARAMETERS
; IDENTIFY PACKET DEVICE
; PACKET
ATAPIOin_nodrdy_withTimeout:
    push ax
    mov ax, ATA_TIMEOUT_NORMAL
    call [set_timeout_count]
    pop ax
ATAPIOin_nodrdy:
    call ATASelectDevice
    jc atapioin_exit.justret
    call ATASendCommand
    jc atapioin_exit
  .after_select:
    call ATAReadData
    call ATAReadStatus
atapioin_exit:
    call ATAPostProcess
  .justret:
    ret


; ATAPIOpacket
; in
; atapi_packet  packet command
; bx            ata portbase
; es:si         read/write data buffer (if extra data exist)
; cx            data transfer size
; ds            cs

ATAPIOpacket:
    call ATASelectDevice
    jc .justret
    mov [atapi_max_bytecount], cx
    xor ax, ax
    mov [atapi_bytecount], ax
    ; mov byte [atareg + IDX_DEVICE_CONTROL], 00001010b
    mov byte [atareg + IDX_FEATURES], 0
    mov byte [atareg + IDX_SECTOR_COUNT], 0         ; TAG
    mov word [atareg + IDX_BYTECOUNT_L], cx
    mov byte [atareg + IDX_COMMAND], 0a0h           ; PACKET command
    call ATASendCommand
    jc .error
    call wait_drq1
    jc .exit                  ; timeout
    push cx
    push si
    push es
    mov si, atapi_packet
    push ds
    pop es
    mov cx, [atapi_packet_size]
    call ATAWriteData
    pop es
    pop si
    pop cx
  .lp:
    call wait_bsy0_atapi
    jc .exit                  ; timeout
    test al, 1
    jnz .error2
    test al, 8h
    jz .fin                   ; DRQ=0
    mov dx, [bx + OFS_INTERRUPT_REASON]
    in al, dx
    test al, 00000101b        ; REL=0,C/D=0...more data to transfer
    jnz .fin
%if 1
    xchg ax, cx
    mov dx, [bx + OFS_BYTECOUNT_H]
    in al, dx
    mov ah, al
    mov dx, [bx + OFS_BYTECOUNT_L]
    in al, dx
    xchg ax, cx
%endif
    or cx, cx                 ; clear CF
    jz .fin
    push cx
    test al, 00000010b        ; I/O
    jz .todev
  .fromdev:
    call [atapi_read_data]    ; (ATAReadData)  I/O = 1
    jmp short .rw_next
  .todev:
    call [atapi_write_data]   ; (ATAWriteData) I/O = 0
  .rw_next:
    pop cx
    add cx, [atapi_bytecount]
    mov [atapi_bytecount], cx
    cmp cx, [atapi_max_bytecount]
    jb .lp
    
  .fin:
    call ATAPIReadStatus
    jnc .exit
  .error:
    cmp ah, 1
    jne .errorexit
  .error2:
    push ax
    push dx
    mov dx, [bx + OFS_INTERRUPT_REASON]
    in al, dx
    mov byte [atapi_interrupt_reason], al
    mov dx, [bx + OFS_ERROR]
    in al, dx
    mov [ata_lasterror], al
    pop dx
    pop ax
  .errorexit:
    stc
  .exit:
%if 0
    call ATAPIPostProcess
%else
    call ATAPostProcess
%endif
  .justret:
    ret


;
; ATAPIcmd_drive
; in
; [ata_drivenum]  drive (0..4)
; atapi_packet  packet command
; es:si         read/write data buffer (if extra data exist)
; cx            max transfer bytes
; ds            cs
;
; bx, dx  break
;

ATAPIcmd_withTimeoutDisc:
    push ax
    mov ax, ATA_TIMEOUT_DISC
    call [set_timeout_count]
    pop ax
    jmp short ATAPIcmd_drive
ATAPIcmd_withTimeout:
    push ax
    mov ax, ATA_TIMEOUT_NORMAL
    call [set_timeout_count]
    pop ax
ATAPIcmd_drive:
    call ATAPrepareSelectDevice
    jmp ATAPIOpacket

clear_atapi_packet:
    push ax
    xor ax, ax
    mov [atapi_packet], ax
    mov [atapi_packet + 2], ax
    mov [atapi_packet + 4], ax
    mov [atapi_packet + 6], ax
    mov [atapi_packet + 8], ax
    mov [atapi_packet + 10], ax
    pop ax
    ret


ATAPIcmd_TestUnitReady:
    call clear_atapi_packet
    push ax
    ; mov byte [atapi_packet], 0
    xor cx, cx
    call ATAPIcmd_withTimeout
    jc .exit
    xor ax, ax			; ZF=1, CF=0
  .exit:
    pop ax
    ret

%ifdef TEST
ATAPIcmd_Inquiry:
    call clear_atapi_packet
    mov byte [atapi_packet], 12h
    mov byte [atapi_packet + 3], ch
    mov byte [atapi_packet + 4], cl
    call ATAPIcmd_withTimeout
    jc .err
    mov ax, [atapi_bytecount]
    ret
  .err:
    mov ax, 0ffffh
    ret
%endif


;---------------------------------------

; dx:ax (LBA) -> ah (M), dl (S), dh (F)
LBAtoAbsMSF_atapi:
    add ax, 150		; lba 00000000h -> 00:02.00
    adc dx, 0
LBAtoMSF_atapi:
    ; check overflow (more than 99:59.74)
    push ax
    push dx
    sub ax, 0ddd0h	; 100:00.00 = 450000 (6ddd0h)
    sbb dx, 6h
    pop dx
    pop ax
    jb .l2
    ; overflow...
    mov dx, 0ffffh
    mov ah, dl
    stc			; CF=1 (error)
    ret
  .l2:
    push cx
    mov cx, 60 * 75
    div cx		; ax = M, dx = 75S+F
    mov ch, al
    mov ax, dx
    mov cl, 75
    div cl		; al = S, ah = F
    mov dx, ax
    xor cl, cl		; CF=0
    mov ax, cx		; al = 0
    pop cx
    ret

; ah (M), dl (S), dh (F) -> dx:ax (LBA)
MSFtoLBA_atapi:
    push cx
    mov al, 60
    xor cx, cx
    mov cl, dh
    mul ah		; ax = 60M
    xor dh, dh
    add ax, dx		; ax = 60M + S
    mov dx, 75
    mul dx		; dx:ax = (60M + S) * 75
    add ax, cx
    adc dx, 0
    pop cx
    ret

AbsMSFtoLBA_atapi:
    mov al, dl
    cmp ax, 2
    jb MSFtoLBA_atapi			; (failproof for less than 00:02.00)
    call MSFtoLBA_atapi
    sub ax, 150
    sbb dx, 0
    ret


; atapi_packet:  dx:ax (LBA) -> ah (M), dl (S), dh (F), al=0
; dos driver  :  dx:ax (LBA) -> dl (M), ah (S), al (F), dh=0
LBAtoMSF_dosdrv:
    call LBAtoMSF_atapi
    xchg ah, dl
    xchg al, dh
    ret

LBAtoAbsMSF_dosdrv:
    call LBAtoAbsMSF_atapi
    xchg ah, dl
    xchg al, dh
    ret


; atapi_packet:  ah (M), dl (S), dh (F) -> dx:ax (LBA)
; dos driver  :  dl (M), ah (S), al (F) -> dx:ax (LBA)
MSFtoLBA_dosdrv:
    xchg ah, dl
    xchg al, dh
    jmp short MSFtoLBA_atapi
AbsMSFtoLBA_dosdrv:
    xchg ah, dl
    xchg al, dh
    jmp short AbsMSFtoLBA_atapi


;---------------------------------------

CDBuf_ATAPIcmd_TestUnitReady:
    call clear_atapi_packet
    ;mov byte [atapi_packet], 0
    push cx
    mov cx, 0
    call cdbuf_atapicmd_common
    pop cx
    jnc cd_check_media_inserted
    cmp byte [atapi_asc], 28h
    je CDBuf_ATAPIcmd_TestUnitReady
    stc
    ret

cd_check_media_inserted:
    cmp byte [cd_media_inserted], 0
    jne .exit
    mov byte [cd_media_changed], 1
    mov byte [cd_need_update], 1
    inc byte [cd_media_inserted]	; cd_media_inserted = 1, CF=0
  .exit:
    ret

CDBuf_ATAPIcmd_CurrentPosition:
    call clear_atapi_packet
    mov byte [atapi_packet], 42h
    mov byte [atapi_packet + 1], 2	; bit1: MSF
    mov byte [atapi_packet + 2], 40h	; SubQ=1
    mov byte [atapi_packet + 3], 01h	; CD-ROM current position
    mov byte [atapi_packet + 8], 16	; data header + current posision blk
    mov cx, 4 + 16
    call cdbuf_atapicmd_common
    jnc cd_check_media_inserted
    ret

CDBuf_ATAPIcmd_ReadCapacity:
    call clear_atapi_packet
    mov byte [atapi_packet], 25h
    xor cx, cx
    mov [cd_buf + 6], cx		; clear block length (for a proof...)
    mov cl, 8
    call cdbuf_atapicmd_common
    jnc .ok
    ret
  .ok:
    xor ax, ax
    xor dx, dx
    cmp word [cd_buf + 6], 0008h	; check block length 0x0800 (2048)
    jne .w_cap
    mov dx, [cd_buf]
    mov ax, [cd_buf + 2]
    xchg dl, dh
    xchg al, ah
    ; inexact value (-75 to +75) for audio-cd
    ; add ax, 75
    ; adc dx, 0
  .w_cap:
    mov [cd_capacity], ax
    mov [cd_capacity + 2],dx
    jmp short cd_check_media_inserted


CDBuf_ATAPIcmd_Inquiry:
    call clear_atapi_packet
    mov cx, 36
    mov byte [atapi_packet], 12h
    mov [atapi_packet + 3], ch
    mov [atapi_packet + 4], cl
    call cdbuf_atapicmd_common
    ret


CDBuf_ATAPIcmd_ModeSelect:
    call clear_atapi_packet
    mov byte [atapi_packet], 55h
    mov byte [atapi_packet + 1], 00010000b
    mov byte [atapi_packet + 8], cl
    xor ch, ch
    jmp short cdbuf_atapicmd_common


CDBuf_ATAPIcmd_ModeSense:
    call clear_atapi_packet
    mov byte [atapi_packet], 5ah
    mov byte [atapi_packet + 1], 00001000b	; Disable Block Descriptor (INF-8090)
    mov byte [atapi_packet + 2], ch
    xor ch, ch
    mov byte [atapi_packet + 7], ch
    mov byte [atapi_packet + 8], cl
    ; fall-thruough
cdbuf_atapicmd_common:
    push si
    push es
    mov si, cd_buf
    push cs
    pop es
    call ATAPIcmd_withTimeout
    pop es
    pop si
    jc atapicmd_common_err
atapicmd_common_noerr:
    mov ax, [atapi_bytecount]
    ret
userbuf_atapicmd_common:
    call ATAPIcmd_withTimeout
    jnc atapicmd_common_noerr
atapicmd_common_err:
    call cdbuf_atapicmd_getlastasc
    mov ax, 0ffffh
    stc
    ret


cdbuf_atapicmd_getlastasc:
    pushf
    call clear_atapi_packet
    mov byte [atapi_packet], 03h
    mov cx, 18
;    mov byte [atapi_packet + 3], ch
    mov byte [atapi_packet + 4], cl
    push si
    push es
    mov si, cd_buf
    mov word [cd_buf + 12], 0		; +12:ASC, +13:ASCQ
    push cs
    pop es
    call ATAPIcmd_withTimeout
    pop es
    pop si
;    mov al, [cd_buf]
;    and al, 0feh
;    cmp al, 0f0h
;    jne .exit
    mov ax, [cd_buf + 12]
    or ax, ax
    jz .exit
    mov word [atapi_asc], ax
    cmp al, 28h
    jne .l2
    mov byte [cd_media_changed], 1
    mov byte [cd_need_update], 1
  .l2:
    cmp al, 3ah
    jne .l3
    mov byte [cd_media_inserted], 0	;
  .l3:
  .exit:
    popf
    ret


;
; Read Sub Channel
; in
; ch bit0-6  format code (01~03)
;    bit7    MSF flag
; cl         track no. 
;
CDBuf_ATAPIcmd_ReadSubChannel:
    call clear_atapi_packet
    mov byte [atapi_packet], 42h
    mov al, ch
    shr al, 6
    and ch, 7fh
    mov byte [atapi_packet + 1], al		; MSF= bit7 of ch1
    mov byte [atapi_packet + 2], 01000000b	; SubQ=1
    mov byte [atapi_packet + 3], ch		; data format
    mov byte [atapi_packet + 6], cl		; track no.
    mov ax, 16 * 100h				; 01h: current positon data format
    dec ch
    je .w_len
    mov ah, 24					; 02h: media catalog number (UPC) data format
    dec ch
    je .w_len
    dec ch					; 03h: track ISRC data format
    je .w_len
    mov byte [atapi_packet + 1], 0		; other value: unknown (SubQ=0: header only)
    mov ah, 4
  .w_len:
    mov word [atapi_packet + 7], ax
    mov cx, 64 ; size_of_cdbuf (todo: set correct data length)
    jmp cdbuf_atapicmd_common


cd_get_all_toc:
    call clear_atapi_packet
    mov byte [atapi_packet], 43h
    mov byte [atapi_packet + 1], 10b		; MSF=1
    ;mov byte [atapi_packet + 2], 0000b		; format (MMC) = 0
    ;mov byte [atapi_packet + 6], 0		; (0 as from first track)
    ; max transfer length : 4 + (8 * 100) = 324h
    mov cx, 4 + 8 * 100
    mov byte [atapi_packet + 7], ch
    mov byte [atapi_packet + 8], cl
    ; mov byte [atapi_packet + 9], 00000000b	; format (SFF8020) = 0
    push si
    push es
    mov si, cd_toc_buf_header
    push cs
    pop es
    mov word [atapi_read_data], ata_shrinked_toc_read
    call ATAPIcmd_withTimeoutDisc
    mov word [atapi_read_data], ATAReadData
    pop es
    pop si
    jc .err
    mov ax, [cd_toc_buf_header]
    ret
  .err:
    call cdbuf_atapicmd_getlastasc
    mov ax, 0ffffh
    ret

ata_shrinked_toc_read:
    mov dx, [bx]    ; bx + OFS_DATA
    push cx
    push di
;    cmp cx, 4
;    jbe .exit
    mov di, si
;    xor ax, ax
;    mov word [es: di], ax
;    mov [cd_toc_pleadout_msf], ax
;    cmp cx, 2
;    jb .drop_rest
    in ax, dx		; TOC data length (big endian)
    xchg al, ah
    stosw
    sub cx, 2
;    mov cx, ax		; do not use TOC length in TOC data (workaround for VBOX passthrough)
    cmp cx, 2
    jb .drop_rest
    ;insw		; first track, last track
    in ax, dx
    stosw
    sub cx, 2
  .lp:			; get info of each track
    cmp cx, 8
    jb .drop_rest
    in ax, dx
    xchg al, ah
    stosb		; adr + control
    in ax, dx		; al = track number (drop it)
    cmp al, 0aah	; leadout track?
    jne .l2
    mov word [cd_toc_pleadout_msf], di
  .l2:
    in ax, dx		; ah = M
    xchg al, ah
    stosb
    in ax, dx		; al = S, ah = F
    stosw
    sub cx, 8
    jmp short .lp
  .drop_rest:
    inc cx
    and cx, 0fffeh
    jz .exit
    shr cx, 1
  .drop_rest_lp:
    ; (I doubt the operation `rep in ax, dx' could be worked on some emulators...)
    in ax, dx
    loop .drop_rest_lp
  .exit:
    pop di
    pop cx
    ret

%if 0
cd_update_leadout_lba:
    call clear_atapi_packet
    mov byte [atapi_packet], 43h
    mov byte [atapi_packet + 1], 00b		; MSF=0 (LBA)
    ;mov byte [atapi_packet + 2], 0000b		; format (MMC) = 0
    mov byte [atapi_packet + 6], 0aah		; (AAh as from lead-out track)
    ; max transfer length : 4 + (8 * 1) = 12
    mov cx, 4 + 8 * 1
    mov byte [atapi_packet + 7], ch
    mov byte [atapi_packet + 8], cl
    ; mov byte [atapi_packet + 9], 00000000b	; format (SFF8020) = 0
    call cdbuf_atapicmd_common
    jc .err
    cmp byte [cd_buf + 4 + 2], 0aah
    jne .err
    mov dx, word [cd_buf + 4 + 4]
    xchg dh, dl
    mov ax, word [cd_buf + 4 + 6]
    xchg ah, al
    push ax
    mov [cd_toc_leadout_lba], ax
    mov [cd_toc_leadout_lba + 2], dx
    or ax, dx					; CF=0; ZF=0 if dx:ax == 0:0
    pop ax
    ret
  .err:
    stc
    ret
%endif


cd_update_device_status:
    mov word [cd_device_status], 0
    mov cx, 2a00h + (8 + 18)
    call CDBuf_ATAPIcmd_ModeSense
    jc .exit
    mov dx, 101000000101b		; cddriver: bit11 no disc, bit9 HSG and redbook, bit2 cooked and raw, bit0 door open
    mov al, byte [cd_buf + 2]
    cmp al, 71h				; sff8020: door open
    je .l2
    and dl, 11111110b			; cddriver: door closed
    cmp al, 70h				; sff8020: door closed, no disc
    je .l2
    mov dh, 10b				; cddriver: disc is present, HSG and redbook
  .l2:
    mov ax, [cd_buf + 8 + 4]		; sff8020: al=page2A[4] ah=page2A[5]
    and al, 1				; sff8020: page2A[4] bit0 AudioPlay
    shl al, 4				; (cddriver: bit4 data or data+audio)
%ifdef SUPPORT_RW_SUBCHANNELS
    and ah, 00000100b			; sff8020: page2A[5] bit2 R-W supported 
    shl ah, 2				; (cddriver: bit12 Supports R-W subchannels)
    or dx, ax
%else
    or dl, al
%endif
    mov ax, [cd_buf + 8 + 6]		; sff8020: al=page2A[6] ah=page2A[7]
    xor al, 10b				; sff8020 page2A[6] bit1 Door Locked -> cddrive bit1 UNlocked
    and ah, 3				; sff8020: page2A[7] bit0 Separate Volume, bit1 Separate Mute
    add ah, 0ffh			; if AH > 0
    adc ah, 0				;   then set bit0 in AH (bit8 in AX)
    and ax, 100000010b			; cddriver: bit8 audio channel manipulation, bit1 door unlocked
    or ax, dx				;           (and clear CF)
    mov [cd_device_status], ax
  .exit:
    ret

cd_update_medium_capacity:
    call CDBuf_ATAPIcmd_ReadCapacity
  .exit:
    ret

cd_update_medium_info:
    call cd_erase_medium_info
    call cd_update_device_status
    jc .exit
    call cd_update_medium_capacity
    call cd_get_all_toc
    xor ax, ax
    xor dx, dx
    mov si, [cd_toc_pleadout_msf]
    or si, si
    jz .w_toc_leadout
    mov ah, [si]
    mov dx, [si + 1]
    call AbsMSFtoLBA_atapi
  .w_toc_leadout:
    mov [cd_toc_leadout_lba], ax
    mov [cd_toc_leadout_lba + 2], dx
    ;cmp [cd_toc_leadout_lba], [cd_capacity]
    push ax
    push dx
    sub ax, [cd_capacity]
    sbb dx, [cd_capacity + 2]
    pop dx
    pop ax
    jb .exit_noerr
    mov [cd_capacity], ax
    mov [cd_capacity + 2], dx
  .exit_noerr:
    clc
  .exit:
    ret

cd_erase_medium_info:
    push ax
    xor ax, ax
    mov word [cd_device_status], ax
    mov word [cd_toc_leadout_lba], ax
    mov word [cd_toc_leadout_lba + 2], ax
    mov word [cd_toc_pleadout_msf], ax
    mov word [cd_toc_buf_header], ax
    mov [cd_capacity], ax
    mov [cd_capacity + 2], ax
    pop ax
    ret


; Open/Close tray
; in
; al  0=stop 2=open(eject) 3=close(load)

ATAPIcmd_LoadCD:
    call clear_atapi_packet
    mov byte [atapi_packet], 01bh		; Start/Stop Unit
;    mov byte [atapi_packet + 1], 1		; immed 
    mov [atapi_packet + 4], al
;    mov byte [atapi_packet + 8], 0		; SLOT (for load/unload CD cmd:A6h)
    xor cx, cx
    call cdbuf_atapicmd_common
    ret

; Lock/Unlock medium
; in
; al  0=unlock 1=lock

ATAPIcmd_Lock:
    call clear_atapi_packet
    mov byte [atapi_packet], 1eh
    mov [atapi_packet + 4], al
    xor cx, cx
    call cdbuf_atapicmd_common
    ret


;
; Read CD
; in
; dx:ax             LBA address
; cx  bit14-bit0    count of blocks
;     bit15         1=raw (2352bytes/sct)  0=cooked (2048bytes/sct)
; es:si             memory address
;

ATAPIcmd_ReadCDLBA:
    call clear_atapi_packet
    mov byte [atapi_packet], 28h	; use Read(10) in cooked mode
    test ch, 80h
    jz .l2
    mov byte [atapi_packet], 0beh	; use Read CD only in raw mode
    mov byte [atapi_packet + 9], 0f8h	; Sync & All Hdrs & User Data + EDC/ECC (2352bytes)
  .l2:
    and cx, 7fffh
    jz .exit
  .lp:
    call atapicmd_readcdlba_1sct
    jc .err
%if 1
    push ax
    mov ax, es
    add ax, 80h				; cooked mode: incr addr 800h (2048byte)
    cmp byte [atapi_packet], 0beh
    jne .l3
    add ax, 13h				; raw mode incr addr 800h + 130h (2352bytes)
  .l3:
    mov es, ax
    pop ax
%else
    add si, 2048
    cmp byte [atapi_packet], 0beh
    jne .l3
    add si, 2352 - 2048			; raw mode incr addr 2352bytes
  .l3:
%endif
    add ax, 1
    adc dx, 0
    loop .lp
  .err:
  .exit:
    ret

atapicmd_readcdlba_1sct:
    push ax
    push cx
    push dx
    xchg al, ah
    mov [atapi_packet + 4], ax
    xchg dl, dh
    mov [atapi_packet + 2], dx
    mov ax, 0100h
    mov [atapi_packet + 7], ax	; transfer length = 1
    mov cx, 0ffffh			; (todo: calc exact transfer bytes and check wraparound)
    call userbuf_atapicmd_common
    pop dx
    pop cx
    pop ax
    ret


;
; Play CD
; in
; ds:si             lba packet (need ds = cs)
;    +0 start LBA
;    +4 end LBA
;
ATAPIcmd_PlayCDLBA:
    call clear_atapi_packet
    mov byte [atapi_packet], 47h
    mov ax, [si]
    mov dx, [si + 2]
    call LBAtoAbsMSF_atapi
    mov [atapi_packet + 3], ah
    mov [atapi_packet + 4], dx
    mov ax, [si + 4]
    mov dx, [si + 6]
    call LBAtoAbsMSF_atapi
    mov [atapi_packet + 6], ah
    mov [atapi_packet + 7], dx
    xor cx, cx
    call ATAPIcmd_withTimeout
    ret

;
; Pause/Resume CD
; in
; al 0=pause 1=resume
ATAPIcmd_PauseCD:
    call clear_atapi_packet
    mov byte [atapi_packet], 4bh
    mov [atapi_packet + 8], al
    xor cx, cx
    call ATAPIcmd_withTimeout
    ret

;---------------------------------------
; device stuff


%if (PRIVATE_STACK_SIZE > 0)
    align 4
prev_stack:
    dd 0
;private_stack:
    times (PRIVATE_STACK_SIZE) db 0cch
private_stack_bottom:
private_stack_count:
    db 00h
%endif

request_header:
    dd 0

Strategy:
    mov word [cs: request_header], bx
    mov word [cs: request_header + 2], es
    retf

Commands:
    pushf
%if (PRIVATE_STACK_SIZE > 0)
    sub byte [cs: private_stack_count], 1
    jnc .cmd_entry
    cli
    mov word [cs: prev_stack], sp
    mov word [cs: prev_stack + 2], ss
    mov sp, cs
    mov ss, sp
    mov sp, private_stack_bottom
    sti
  .cmd_entry:
%endif
    cld
    pusha
    push ds
    push es
    push cs
    pop ds
    les di, [request_header]
    mov al, byte [es: di+2]
    mov si, cmd_table_00
    cmp al, 80h
    jb .l2
    mov si, cmd_table_80
  .l2:
    lea bx, [si + 2]
    cmp al, byte [si]
    jb .call_entry
    cmp al, byte [si + 1]
    ja .call_entry
    add bx, 2
    sub al, byte [si]
    cbw
    add ax, ax
    add bx, ax
  .call_entry:
    mov cx, word [es: di + 18]
    les di, [es: di + 14]
    call word [bx]
    lds di, [request_header]
    mov word [di + 3], ax
  .exit:
    pop es
    pop ds
    popa
%if (PRIVATE_STACK_SIZE > 0)
    add byte [cs: private_stack_count], 1
    jnc .just_ret
    cli
    mov sp, word [cs: prev_stack]
    mov ss, word [cs: prev_stack + 2]
    sti
  .just_ret:
%endif
    popf
    retf


Command_ReadLongPrefetch:
Command_Seek:
Command_Unknown:
    mov ax, 8103h
    ret


;Command_IOCTLin:
;Command_IOCTLout:
Command_NotReady:
    mov ax, 8102h
    ret

Command_InputFlush:
Command_OutputFlush:
Command_Open:
Command_Close:
Command_Done:
    mov ax, 0100h
    ret

Command_IOCTLout:
    mov si, ioctl_table_out
    jmp short ioctl_inout
Command_IOCTLin:
    mov si, ioctl_table_in
ioctl_inout:
    mov al, byte [es: di]
    xor ah, ah
    cmp ax, word [si]
    ja Command_Unknown
    shl ax, 2
    add si, ax
    cmp cx, word [si + 2]
    ;jne Command_Unknown
    jb Command_Unknown
    call word [si + 4]
    ret

ioctl_table_in:
    dw 15
    dw 5, CD_DeviceHeader
    dw 6, CD_LocationOfHead
    dw 1, CD_Unknown            ; reserved
    dw 1, CD_Unknown            ; Error Statistics
    dw 9, CD_AudioChannelInfo
    dw 130, CD_ReadDriveBytes
    dw 5, CD_DeviceStatus
    dw 4, CD_ReturnSectorSize
    dw 5, CD_ReturnVolumeSize
    dw 2, CD_MediaChanged
    dw 7, CD_AudioDiscInfo
    dw 7, CD_AudioTrackInfo
    dw 11, CD_AudioQChannelInfo
    dw 13, CD_AudioSubChannelInfo
    dw 11, CD_UPCCode
    dw 10, CD_AudioStatusInfo	; note: not 11 but 10 for Win3.1 MCICDA.drv
    

ioctl_table_out:
    dw 5
    dw 1, CD_EjectDisk
    dw 2, CD_LockDoor
    dw 1, CD_ResetDrive
    dw 9, CD_AudioChannelControl
    dw 1, CD_WriteDeviceControlString
    dw 1, CD_CloseTray

cmd_table_00:
    db 1, 14			; from 1 to 14
    dw Command_Unknown		; out of region
    dw Command_Unknown		;  1 Media Check
    dw Command_Unknown		;  2 Build BPB
    dw Command_IOCTLin		;  3 IOCTL input
    dw Command_Unknown		;  4 Input
    dw Command_Unknown		;  5 Non destructive input No wait
    dw Command_Unknown		;  6 Input status
    dw Command_InputFlush	;  7 Input flush
    dw Command_Unknown		;  8 Output
    dw Command_Unknown		;  9 Output with verify
    dw Command_Unknown		; 10 Output status
    dw Command_OutputFlush	; 11 Output flush
    dw Command_IOCTLout		; 12 IOCTL output
    dw Command_Open		; 13 Device open
    dw Command_Close		; 14 Device Close

cmd_table_80:
    db 128, 136			; (from 128 to 136)
    dw Command_Unknown		; out of region
    dw Command_ReadLong		; 128 Read Long
    dw Command_Unknown          ; 129 (reserved)
    dw Command_ReadLongPrefetch ; 130 Read Long Prefetch
    dw Command_Seek             ; 131 Seek
    dw Command_Play             ; 132 Play Audio
    dw Command_Stop             ; 133 Stop Audio
    dw Command_Unknown          ; 134 Write Long
    dw Command_Unknown          ; 135 Write Long Verify
    dw Command_Resume           ; 136 Resume Audio

;---------------------------------------

cdDriveNum:
    mov ax, [cd_drvnum]
    test ax, 00fch	; check within 0..3 (and clear CF)
    jnz .no_unit
    and ax, 3
;    mov [ata_drivenum], ax
    ret
  .no_unit:
    mov ax, 8101h       ; unknown unit
    stc
    ret


cdCheckDiscIn:
%if 1
    call cdDriveNum
    jc .errexit
  .l1:
    call CDBuf_ATAPIcmd_TestUnitReady
    jc .chk_ready
    cmp byte [cd_need_update], 0
    jne .update_disc_info
    ret
  .update_disc_info:
    call cd_update_medium_info
    mov byte [cd_need_update], 0
    jc .err_notready
    ret
  .chk_ready:
    cmp byte [atapi_asc], 28h		; (media changed?)
    je .l1
  .err_notready:
    mov ax, 8102h			; not ready
  .errexit:
    stc
    ret
%else
  .l0:
    call CDBuf_ATAPIcmd_TestUnitReady
    jnc .l2
    cmp byte [atapi_asc], 28h	; (media changed?)
    je .l0
    stc
    ret
  .l2:
    cmp byte [cd_need_update], 0
    jne .update_info
    ret
  .update_info:
    call cd_update_medium_info
    mov byte [cd_need_update], 0
    ret
%endif


CD_DeviceHeader:
    mov word [es: di + 1], device_header
    mov word [es: di + 3], cs
cd_done:
    mov ax, 0100h
    ret

CD_ReturnSectorSize:
    mov ax, 2048
    cmp byte [es: di + 1], 0	; cooked?
    je .ok
    mov ax, 2352
    cmp byte [es: di + 1], 1	; raw?
    je .ok
    xor ax, ax			; (as far as I tested,) OAKCDROM will return 0 for unknown mode without error
  .ok:
    mov word [es: di + 2], ax
    mov ax, 0100h
    ret

%ifdef SUPPORT_DRIVE_BYTES
CD_ReadDriveBytes:
    ; (return vendor and product name of the drive for a test).
    call cdDriveNum
    jc .exit
    call CDBuf_ATAPIcmd_Inquiry
    jc .cmderr
    mov si, cd_buf + 8
    mov cx, 8 + 16 + 4
    mov byte [es: di + 1], cl
    add di, 2
    rep movsb
    mov ax, 0100h
    ret
  .cmderr:
    mov ax, 8102h	; unit not ready
  .exit:
    ret
%endif


CD_DeviceStatus:
    call cdDriveNum
    jnc .l1
    ret					; unknown unit (8101h)
  .l1:
    mov ch, 2ah
    mov cl, 8 + 18
    call CDBuf_ATAPIcmd_ModeSense
    jc .cmderr
    mov dx, 101000000101b		; cddriver: bit11 no disc, bit9 HSG and redbook, bit2 cooked and raw, bit0 door open
    mov al, byte [cd_buf + 2]
    cmp al, 71h				; sff8020: door open
    je .l2
    and dl, 11111110b			; cddriver: door closed
    cmp al, 70h				; sff8020: door closed, no disc
    je .l2
    mov dh, 10b				; cddriver: disc is present, HSG and redbook
  .l2:
    mov ax, [cd_buf + 8 + 4]		; sff8020: al=page2A[4] ah=page2A[5]
    and al, 1				; sff8020: page2A[4] bit0 AudioPlay
    shl al, 4				; (cddriver: bit4 data or data+audio)
%ifdef SUPPORT_RW_SUBCHANNELS
    and ah, 00000100b			; sff8020: page2A[5] bit2 R-W supported 
    shl ah, 2				; (cddriver: bit12 Supports R-W subchannels)
    or dx, ax
%else
    or dl, al
%endif
    mov ax, [cd_buf + 8 + 6]		; sff8020: al=page2A[6] ah=page2A[7]
    xor al, 10b				; sff8020 page2A[6] bit1 Door Locked -> cddrive bit1 UNlocked
%ifdef SUPPORT_AUDIO_CHANNEL
    and ah, 3				; sff8020: page2A[7] bit0 Separate Volume, bit1 Separate Mute
    add ah, 0ffh			; if AH > 0
    adc ah, 0				;   then set bit0 in AH (bit8 in AX)
    and ax, 100000010b			; cddriver: bit8 audio channel manipulation, bit1 door unlocked
    or dx, ax
%else
    and al, 00000010b			; cddriver: bit1 door unlocked
    or dl, al
%endif
    mov [cd_device_status], dx
    mov word [es: di + 1], dx
    mov word [es: di + 3], 0
    mov ax, 0100h
    ret
  .cmderr:
    cmp byte [atapi_asc], 28h
    jne .cmderr2
    mov byte [cd_media_changed], 1
    mov byte [cd_need_update], 1
    jmp short .l1
  .cmderr2:
    mov ax, 8102h
  .exit:
    ret




%ifdef SUPPORT_CD_PLAY

%ifdef SUPPORT_AUDIO_CHANNEL
CD_AudioChannelInfo:
    call cdDriveNum
    jnc .l1
    ret					; unknown unit (8101h)
  .l1:
    mov ch, 0eh
    mov cl, 8 + 16
    call cdbuf_atapicmd_modesense_with_changed
    jc .cmderr
    xor cx, cx
    mov si, cd_buf + 8 + 8
    inc di
  .lp:
    mov ax, [si + 20h]			; get mask
    or al, al				; check mask for channel selection
    jnz .l01
    xor ax, ax				; channel not supported if mask==0
    jmp short .lw
  .l01:
    and ax, [si]
    and al, 0fh				; mute? (channel selection == 0000b)
    jnz .l02
    mov ax, cx				; mute: al = chennel, ah = 0
    jmp short .lw
  .l02:					; channel selection == 1or2 as channel 0or1
    cmp al, 3
    jae .l03
    dec al
    jmp short .lw
  .l03:					; channel selection == 0011b as mono (0+1)
    jne .l04
    mov al, cl
    and al, 1
    jmp short .lw
  .l04:
    cmp al, 4				; channel selection == 0100b as channel 2
    jne .l08
    mov al, 2
    jmp short .lw
  .l08:
    cmp al, 8				; channel selection == 1000b as channel 3
    jne .lother
    mov al, 3
    jmp short .lw
  .lother:
    mov ax, cx
  .lw:
    stosw
    add si, 2
    inc cl
    cmp cl, 4
    jb .lp
    mov ax, 0100h
    ret
  .cmderr:
    mov ax, 8102h
    ret

CD_AudioChannelControl:
    call cdDriveNum
    jnc .l1
    ret					; unknown unit (8101h)
  .l1:
    mov ch, 0eh
    mov cl, 8 + 16
    call cdbuf_atapicmd_modesense_with_changed
    jc .cmderr
    mov cx, 4
    mov si, cd_buf + 8 + 8
    inc di
  .lp:
    mov ax, [es: di]
    and al, 3				; (todo: check bound)
    inc al				; 0->1(channel0) 1->2(channel1)
    cmp al, 2
    jbe .l2
    cmp al, 3
    mov al, 4				; 2->4(channel2)
    je .l2
    mov al, 8				; 3->8(channel3)
  .l2:
;    or ah, ah
;    jnz .l3
;    mov al, 0				; volume 0 : mute
  .l3:
    mov [si], ax
    add si, 2
    add di, 2
    loop .lp
    mov cx, 8
    mov si, cd_buf
    push cx
  .lp_wmask:
    mov al, [si + 20h + 8 + 8]
    and [si + 8 + 8], al
    add si, 1
    loop .lp_wmask
    pop cx
    ;mov ch, 0eh
    mov cx, 8 + 16
    call CDBuf_ATAPIcmd_ModeSelect	; todo
    jc .cmderr
    mov ax, 0100h
    ret
  .cmderr:
    mov ax, 8102h
    ret

; cdbuf +00h ... -> mode sense page control=00b:current
; cdbuf +20h ... -> mode sense page control=01b:changeable (mask) value
cdbuf_atapicmd_modesense_with_changed:
    call CDBuf_ATAPIcmd_TestUnitReady
    push si
    push di
    push es
    push ds
    pop es
  .get_changeable:
    push cx
    and ch, 00111111b
    or ch, 01000000b			; changeable values
    call CDBuf_ATAPIcmd_ModeSense
    pop cx
    jnc .copy_changeable
;    cmp byte [atapi_asc], 28h		; media chaned -> retry
;    je .get_changeable
    push cx
    mov di, cd_buf
    xor ch, ch
    mov al, 0ffh
    rep stosb
    pop cx
  .copy_changeable:
    push cx
    mov si, cd_buf
    mov di, cd_buf + 32
    xor ch, ch
    rep movsb
    pop cx
    pop es
    pop di
    pop si
    jmp CDBuf_ATAPIcmd_ModeSense

%endif		; SUPPORT_AUDIO_CHANNEL

CD_AudioDiscInfo:
    call cdCheckDiscIn
    jc .exit
    mov ax, [cd_toc_first_track]	; al = 1st, ah = last
    mov word [es: di + 1], ax
    mov si, [cd_toc_pleadout_msf]
    mov ax, [si + 1]
    xchg al, ah
    mov word [es: di + 3], ax
    mov al, [si]
    xor ah, ah
    mov [es: di + 5], ax
    mov ax, 0100h
  .exit:
    ret


CD_AudioTrackInfo:
    call cdCheckDiscIn
    jc .exit
    mov ax, 810ch
    mov dl, byte [es: di + 1]
    cmp dl, [cd_toc_last_track]
    ja .exit
    sub dl, [cd_toc_first_track]
    jb .exit
    mov al, 4
    mul dl
    add ax, cd_toc_buf
    mov si, ax
    lodsb			; control info
    rol al, 4
    mov byte [es: di + 6], al
    mov byte [es: di + 5], 0
    lodsw			; al = M, ah = S
    xchg al, ah
    mov word [es: di + 3], ax
    lodsb			; F
    mov byte [es: di + 2], al
    mov ax, 0100h
  .exit:
    ret


CD_AudioQChannelInfo:
    call cdCheckDiscIn
    jc cd_checkbusy.exit
    mov cx, 8100h			; CD-ROM position (MSF=1), track=0
    call CDBuf_ATAPIcmd_ReadSubChannel
    jc cd_checkbusy.err_notready
    ;cmp [cd_buf + 4], 1
    ;jne .err
    mov al, [cd_buf + 5]
    ror al, 4
    test al, 0fh
    jz cd_checkbusy.err_notready
    push di
    inc di
    stosb
    mov ax, [cd_buf + 6]		; al = track, ah = index
    stosw
    mov ax, [cd_buf + 13]		; al = M ah = S
    stosw
    mov al, [cd_buf + 15]		; F
    xor ah, ah
    stosw
    mov ax, [cd_buf + 9]		; al = AM, ah = AS
    stosw
    mov al, [cd_buf + 11]		; AF
    stosb
    pop di
cd_checkbusy:
    mov ax, 0100h
    cmp byte [cd_buf + 1], 11h		; set BUSY bit in playing audio
;    je .busy
;    cmp byte [cd_buf + 1], 12h		; and in pause
    jne .exit
  .busy:
    or ah, 2h
  .exit:
    ret
  .err_notready:
    mov ax, 8102h
    ret

; update cdplay_start
; in
; cl bit0  1=update in playing
;    bit1  1=update in pause
;
;    bit7  1=set RedBook addr into dword[es: di + 3]
; result
; cy       0 = success
; ch       cd audio state [cd_buf + 1]
cd_update_cdplay_start:
    push cx
    mov cx, 8100h
    call CDBuf_ATAPIcmd_ReadSubChannel
    pop cx
    mov ch, [cd_buf + 1]
    jc .exit
  .chk_playing:
    test cl, 1
    jz .chk_pause
    cmp ch, 11h
    je .get_msf
  .chk_pause:
    test cl, 2
    jz .get_prevmsf
    cmp ch, 12h
    je .get_msf
  .get_prevmsf:
    mov ax, [cdplay_start_lba]
    mov dx, [cdplay_start_lba + 2]
    call LBAtoAbsMSF_dosdrv
    jmp short .chk_write
  .get_msf:
    mov ax, [cd_buf + 4 + 6]		; (al = S, ah = F)
    xchg al, ah				; -> al = F, ah = S
    xor dh, dh
    mov dl, [cd_buf + 4 + 5]		; dl = M
  .chk_write:
    test cl, 80h
    jz .update_l2
    mov [es: di + 3], ax
    mov [es: di + 5], dx
  .update_l2:
    call AbsMSFtoLBA_dosdrv
    mov [cdplay_start_lba], ax
    mov [cdplay_start_lba + 2], dx
  .exit:
    ret


CD_AudioStatusInfo:
    xor ax, ax
    mov [es: di + 1], ax
    mov [es: di + 3], ax
    mov [es: di + 5], ax
    mov [es: di + 7], ax
    mov [es: di + 9], al
    cmp cx, 10
    jbe .l0
    mov [es: di + 9], ah
  .l0:
    call cdCheckDiscIn
    jc .exit
    mov ax, [cdplay_end_lba]
    or ax, [cdplay_end_lba + 2]
    jz .exit
    mov cl, 82h				; update in only pause, write addr to es:di + 3
    call cd_update_cdplay_start
    mov ax, [cdplay_end_lba]
    mov dx, [cdplay_end_lba + 2]
    call LBAtoAbsMSF_dosdrv
    mov [es: di + 7], ax
    mov [es: di + 9], dl	; dx
    cmp ch, 11h
    jne .exit
    mov ax, 0300h
    ret
  .exit:
    mov ax, 0100h
    ret


cdplay_packet:
cdplay_start_lba:
    dd 0
cdplay_end_lba:
    dd 0



Command_Play:
    call cdCheckDiscIn
    jc .err_notready
    les di, [request_header]
    mov cl, [es: di + 13]
    test cl, 0feh
    jnz .err_param
    mov ax, [es: di + 14]
    mov dx, [es: di + 16]
    test cl, 1
    jz .l2
    call AbsMSFtoLBA_dosdrv
  .l2:
    mov si, cdplay_packet
    mov [si], ax
    mov [si + 2], dx
    add ax, [es: di + 18]
    adc dx, [es: di + 20]
    mov [si + 4], ax
    mov [si + 6], dx
    push cs
    pop es
    call ATAPIcmd_PlayCDLBA
    mov ax, 0300h
    jnc .just_ret
  .err_notready:
    mov ax, 8102h
  .err_param:
    mov ax, 810ch
  .just_ret:
    ret


Command_Stop:
    call cdCheckDiscIn
    jc .set_stop
    mov cx, 8100h			; CD-ROM position (MSF=1), track=0
    call CDBuf_ATAPIcmd_ReadSubChannel
    jc .set_stop
    cmp byte [cd_buf + 1], 11h		; playing?
    je .do_pause
  .do_stop:				; pause -> stop
    mov al, 0
    call ATAPIcmd_LoadCD
  .set_stop:
    xor ax, ax
    mov [cdplay_start_lba], ax
    mov [cdplay_start_lba + 2], ax
    mov [cdplay_end_lba], ax
    mov [cdplay_end_lba + 2], ax
    jmp short .exit
  .do_pause:
    mov cl, 01h				; update cdplay_start_lba in playing
    call cd_update_cdplay_start
    mov ax, 0				; playing -> pause
    call ATAPIcmd_PauseCD
  .exit:
    mov ax, 0100h
    ret
    

Command_Resume:
    mov ax, [cdplay_end_lba]
    or ax, [cdplay_end_lba + 2]
    jz .err_noresume
    mov ax, 1
    call ATAPIcmd_PauseCD
    mov ax, 0300h
    jnc .ret
  .err_noresume:
    mov ax, 810Ch
  .ret:
    ret


%endif	; SUPPORT_CD_PLAY


CD_LocationOfHead:
    call cdCheckDiscIn
    jc .exit
    ; todo (neccdd): calc MSFtoLBA by myself
    mov al, byte [es: di + 1]
    test al, 0feh
    jnz .err_param
    shl ax, 15
    or ah, 1
    mov cx, ax				; ch = 01/81 (CD-ROM position) cl = 0
    call CDBuf_ATAPIcmd_ReadSubChannel
    mov ax, 8102h
    jc .exit
    mov ax, [cd_buf + 10]
    xchg al, ah
    mov [es: di + 2], ax
    mov ax, [cd_buf + 8]
    xchg al, ah
    mov [es: di + 4], ax
    mov ax, 0100h
    cmp byte [cd_buf + 1], 11h		; set BUSY bit in playing audio
    jne .exit
    or ah, 2h
  .exit:
    ret
  .err_param:
    mov ax, 810ch
    ret


CD_ReturnVolumeSize:
    call cdCheckDiscIn
    jc .exit
    mov ax, [cd_toc_leadout_lba]
    mov dx, [cd_toc_leadout_lba + 2]
    push ax
    push dx
    or ax, dx			; if cd_toc_leadlba == 0?
    jz .l1
    sub ax, 0ddd0h		;   or cd_toc_leadlba >= 6ddd0h? (same or over 100min)
    sbb dx, 6
  .l1:
    pop dx
    pop ax
    jb .l2
    mov ax, [cd_capacity]	; then use cd_capaticy (the medium is not CD, probably)
    mov dx, [cd_capacity + 2]
  .l2:
    mov word [es: di + 1], ax
    mov word [es: di + 3], dx
    mov ax, 0100h
  .exit:
    ret


CD_MediaChanged:
    call cdCheckDiscIn
;    mov ax, 8102h
%if 1
    mov ax, 0100h
    cmp byte [cd_media_changed], 0
    je .l2
    mov dl, 0ffh
    mov byte [cd_media_changed], 0
    jmp short .w_result
  .l2:
    mov dl, [cd_media_inserted]		; 0 (no disc) or 1 (inserted)
    cmp dl, 0
    jne .w_result
    mov ax, 8102h
%else
    mov dl, [cd_media_inserted]		; 0 (no disc) or 1 (inserted)
    jc .w_result			; (as far as I tested) when disc is not mounted, OAKCDROM will modify media byte to 0 and return as error (8102h)
    mov ax, 0100h
    cmp al, [cd_media_changed]
    je .w_result
    mov dl, 0ffh
    mov [cd_media_changed], al		; read and clear internal change state
%endif
  .w_result:
    mov [es: di + 1], dl
    ret



CD_ResetDrive:
    ;todo
    ; mov byte [cd_need_update], 1
    jmp cd_done

CD_EjectDisk:
    call CDBuf_ATAPIcmd_TestUnitReady
    mov ax, 2		; unload media
    call ATAPIcmd_LoadCD
    mov ax, 0100h
    jnc cd_chkerr_notready.ret
    cmp byte [atapi_asc], 3ah
    je cd_chkerr_notready.ret
cd_chkerr_notready:
    mov ax, 8102h
  .ret:
    ret

CD_CloseTray:
    call CDBuf_ATAPIcmd_TestUnitReady
    mov ax, 3
    call ATAPIcmd_LoadCD
    jnc .noerr
    cmp byte [atapi_asc], 28h
    jne .chkasc
    call CDBuf_ATAPIcmd_TestUnitReady
    jnc .noerr
  .chkasc:
    cmp byte [atapi_asc], 3ah
    je cd_chkerr_notready
  .noerr:
    mov ax, 0100h
    ret
cd_chkerr_810c:
    mov ax, 810ch
    ret

; todo
CD_LockDoor:
    call cdCheckDiscIn
    jc .ret
    mov al, [es: di + 1]
    test al, 0feh
    jnz cd_chkerr_810c
    and al, 1
    push es
    push ds
    pop es
    call ATAPIcmd_Lock
    pop es
    mov ax, 0100h
    jc cd_chkerr_810c
  .ret:
    ret


; fallback to command error (8103h)

%ifndef SUPPORT_CD_PLAY
CD_AudioDiscInfo:
CD_AudioTrackInfo:
CD_AudioQChannelInfo:
CD_AudioStatusInfo:
Command_Play:
Command_Stop:
Command_Resume:
%endif
%ifndef SUPPORT_AUDIO_CHANNEL
CD_AudioChannelInfo:
CD_AudioChannelControl:
%endif

CD_UPCCode:

%ifndef SUPPORT_RW_SUBCHANNELS
CD_AudioSubChannelInfo:
%endif
%ifndef SUPPORT_DRIVE_BYTES
CD_ReadDriveBytes:
%endif
CD_WriteDeviceControlString:
CD_Unknown:
    mov ax, 8103h
    ret


Command_ReadLong:
    call cdCheckDiscIn
    jc .just_ret
    push ds
    lds si, [request_header]
    mov cx, [si + 18]
    and cx, 7fffh			; (just for a proof)
  .l1:
    mov al, byte [si + 24]		; data read mode
    test al, 0feh			; valid only 0 (cooked) or 1 (raw)
    jnz .err_paramchk
    shl al, 7
    or ch, al				; cx bit15 = raw/cooked
    mov al, byte [si + 13]		; addressing mode
    test al, 0feh			; valid only 0 (LBA) or 1 (RedBook)
    jnz .err_paramchk
    test al, 1
    mov ax, [si + 20]			; starting sector
    mov dx, [si + 22]
    jz .l2
    call AbsMSFtoLBA_dosdrv
  .l2:
    les si, [si + 14]
    pop ds
    call ATAPIcmd_ReadCDLBA
    pushf
    push ds
    lds si, [request_header]
    sub word [si + 18], cx		; store actual count of read sector
    pop ds
    popf
    jc .err_read
    mov ax, 0100h
  .just_ret:
    ret
  .err_read:
    mov ax, 810bh
    ret

  .err_paramchk:
    pop ds
    mov ax, 810ch
    ret



    align 16
TSR_bottom:
;


;---------------------------------------
; ATAIdentifyDevice
; in
; (al    drive (0..3))
; [ata_drivenum] drive (0..3))
; ds    = cs
; es:si buffer (up to 512bytes)
; cx    bytes of buffer (2..512, must be even)
;
; out
; ax    result
;       -1 timeout
;       bit8=1  error
;            0  al = 00..1Fh atapi device type
;                    FF      not atapi (ata)
; cy    set if error
; buffer512
;       device information (if not error)
; bx,dx will be modified
;

ATAIdentifyDevice:
    push ax
    mov ax, ATA_TIMEOUT_IDENTIFY
    call [set_timeout_count]
    pop ax
    
    call ATAPrepareSelectDevice
    call ATASelectDevice
    jc .justret                               ; timeout or error
    
    mov byte [atareg + IDX_COMMAND], 0a1h    ; IDENTIFY PACKET DEVICE
    call ATASendCommand
    jnc .readdata
    call ATAReadStatus
    call wait_drdy1
    jc .exit                                  ; timeout
    mov byte [atareg + IDX_COMMAND], 0ech    ; IDENTIFY DEVICE
    call ATASendCommand
    jc .exit
    test al, 20h                              ; check DF=0 for a proof
    jnz .nodevice
  .readdata:
    ;call ATAReadData
    mov dx, [bx]    ; bx + OFS_DATA
    push cx
    push si
    push di
    mov di, si
    cmp cx, 512
    jbe .rd_01
    mov cx, 512
  .rd_01:
    inc cx
    shr cx, 1
    mov si, 256
    sub si, cx
    ;pushf
    rep insw
    ;popf
    ;jbe .readdata_end
    mov cx, si
    jcxz .readdata_end
  .readdrop_lp:
    in ax, dx
    loop .readdrop_lp
  .readdata_end:
    pop di
    pop si
    pop cx

    call ATAReadStatus
    jc .exit
    ; quick check data correctness
    ; (model infomation chars are in ascii?)
    cmp cx, 27 * 2 + 2
    jb .isata_atapi
    mov ax, word [es: si + (27 * 2)]		; Model Number (word 27~46)
    cmp ah, 20h
    jb .nodevice
    cmp ah, 7fh
    jae .nodevice
    cmp al, 20h
    jb .nodevice
    cmp al, 7fh
    jae .nodevice
  .isata_atapi:
    ; quick check bit15,14 in word0
    ; bit15 14 
    ;     1  0 atapi
    ;     0  x ata
    mov ax, word [es: si]
    or ax, ax
    je .nodevice
    ;cmp ax, 0ffffh
    ;je .nodevice
    test ah, 80h
    jz .ata_or_atapi      ; bit15=0: maybe ata
    test ah, 40h
    jz .ata_or_atapi      ; bit15=1,bit14=0: maybe atapi
  .nodevice:
    mov ax, 01ffh
    stc
    jmp short .exit
  .ata_or_atapi:
    shr ax, 5
    shr al, 3
    and ah, 110b
    cmp ah, 100b
    je .noerr
    mov al, 0ffh
  .noerr:
    xor ah, ah
  .exit:
    call ATAPostProcess
  .justret:
    ret



isNEC98:        ; detect IBMPC/NEC98 by checking int 1Ah AH=0 has (no) op
    push ax
    push cx
    push dx
    mov cx, 0ffffh
    mov ah, 0
    int 1ah
    cmp cx, 0ffffh
    jne .ibm
    mov cx, 0fffeh
    mov ah, 0
    int 1ah
    cmp cx, 0fffeh
    stc
    mov byte [is_nec98], 1
    jz .exit
  .ibm:
    or al, al   ; just for ensure CF=0
  .exit:
    pop dx
    pop cx
    pop ax
    ret


SwitchToNEC98:
    mov word [cs: ata_wait_bsyvalid], Wait600ns_nec98
    mov word [cs: ata_select_ports], ATASelectPorts_nec98
    mov word [cs: set_timeout_count], SetTimeoutCount_nec98
    mov word [cs: is_timeout], isTimeout_nec98
    ret


;-------------------------------
; display message

;

do_verbose:
    db 1



putc_verbose:
    cmp byte [do_verbose], 0
    jne putc
    ret
putc:
    push ax
    push dx
    mov ah, 2
    mov dl, al
    int 21h
    pop dx
    pop ax
    ret

putn_verbose:
    cmp byte [do_verbose], 0
    jne putn
    ret
putn:
    push ax
    mov al, 13
    call putc
    mov al, 10
    call putc
    pop ax
    ret

puth4:
    push ax
    and al, 0fh
    add al, '0'
    cmp al, '9'
    jbe .putc
    add al, 'A' - '9' - 1
  .putc:
    call putc
    pop ax
    ret

puth8:
    ror al, 4
    call puth4
    ror al, 4
    jmp short puth4

puth16:
    xchg ah, al
    call puth8
    xchg ah, al
    jmp short puth8


putmsg_verbose:
    cmp byte [do_verbose], 0
    jne putmsg
    ret
putmsg:
    push ax
    mov ah, 9
    int 21h
    pop ax
    ret

putmsg_ln:
    call putmsg
    jmp short putn

putmsg_cx:
    push cx
    push si
  .lp:
    lodsb
    call putc
    loop .lp
    pop si
    pop cx
    ret




;--------------------------------------------------------------



DetectATAPICD:
    push bx
    xor ax, ax
  .lp00:
    mov [ata_drivenum], ax
    mov [cd_drvnum], al
    mov dx, msgPrimary
    test al, 10b
    jz .lp01m2
    mov dx, msgSecondary
  .lp01m2:
    call putmsg_verbose
    mov dx, msgMaster
    test al, 01b
    jz .lp01m3
    mov dx, msgSlave
  .lp01m3:
    call putmsg_verbose
    mov si, ata_identify_buf
    mov cx, 256
    call ATAIdentifyDevice
    jnc .check
  .nodev:
    mov dx, msgNone
    call putmsg_verbose
  .next:
    call putn_verbose
    mov ax, [ata_drivenum]
    inc ax
    cmp ax, 4
    jb .lp00
    mov ax, 0ffffh
    mov [ata_drivenum], ax
    mov [cd_drvnum], al
    stc
  .exit:
    mov ax, [ata_drivenum]
    pop bx
    ret
  ; check ATA(PI) device
  .check:
    cmp ax, 1fh
    jbe .atapi_inquiry
    mov cx, 20
    add si, 27 * 2
  .ata_devname_lp:
    lodsw
    xchg al, ah
    call putc_verbose
    xchg al, ah
    call putc_verbose
    loop .ata_devname_lp
    mov dx, msgATA
    call putmsg_verbose
    jmp short .next
  .atapi_inquiry:
    ; to check NECCD, use inquiry command
    call CDBuf_ATAPIcmd_Inquiry
    jc .nodev
    mov si, cd_buf
    push si
    add si, 8
    mov cx, 8 + 16 + 4
  .atapi_devname_lp:
    lodsb
    call putc_verbose
    loop .atapi_devname_lp
    pop si
    mov dx, msgATAPI
    mov al, byte [si]
    and al, 1fh
    cmp al, 5		; CD-ROM?
    pushf		; preserve "CD-ROM?" flags
    jne .atapi_put
    mov dx, msgATAPICD
  .atapi_put:
    mov cx, 40 - 28
    mov al, ' '
  .atapi_put0:
    call putc_verbose
    loop .atapi_put0
    call putmsg_verbose
    popf		; retrieve "CD-ROM?" flag
    jne .next		; (continue if not CD-ROM)
    add si, 8
    mov di, strNECCD260
    mov cx, 8 + 16
    repe cmpsb
    jne .atapi_l2
    lodsb		; 1st revision str
    cmp al, '2'
    ja .atapi_l2
    mov byte [isNECCD260], al
  .atapi_l2:
    call putn_verbose
    clc
    jmp .exit

msgPrimary:
    db '  Port#0 ', eos
msgSecondary:
    db '  Port#1 ', eos
msgMaster:
    db 'Master : ', eos
msgSlave:
    db 'Slave  : ', eos
msgNone:
    db '  (no device)', eos
msgATA:
    db ' (ATA)', eos
msgATAPI:
    db ' (ATAPI)', eos
msgATAPICD:
    db ' (ATAPI CD-ROM)', eos
strNECCD260:
    db 'NEC     ', 'CD-ROM DRIVE:260'	; 8 (vendor) + 16 (product) = 24chars


Init_Commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push ds
    push es
    
    mov ax, cs
    mov ds, ax
    mov es, ax
    cld
    mov word [device_commands_entry], Commands
    mov dx, msgOpening1
    call putmsg
    mov si, devnamePCAT
    call isNEC98
    jnc .l2
    mov si, devnamePC98
    call SwitchToNEC98
  .l2:
    mov cx, 8
    mov di, devname
    rep movsb
    mov dx, si
    call putmsg
    mov dx, msgOpening2
    call putmsg_ln
    
    les bx, [request_header]
    les si, [es: bx + 18]
    call GetParams
    jnc .l3
    mov dx, errDeviceOption
    call putmsg_ln
    jmp short .not_resident

  .l3:
    push cs
    pop es
    call DetectATAPICD
    jc .err_nocd
    
  .resident:
    mov dx, msgDeviceName
    call putmsg
    mov si, devname
    mov dx, si
    mov di, device_name
    push cs
    pop es
    mov cx, 8
    rep movsb
    call putmsg_ln
    
    lds bx, [request_header]
    mov byte [bx + 13], 1
    mov word [bx + 14], TSR_bottom
    mov word [bx + 16], cs
    mov word [bx + 3], 0100h
    jmp short .exit
    
  .err_nocd:
    mov dx, errNoCDROM
  .err_msgdisp:
    call putmsg_ln
  .not_resident:
    lds bx, [request_header]
    mov byte [bx + 13], 0
    mov word [bx + 14], 0
    mov word [bx + 16], cs
    mov word [bx + 3], 810ch
    ; mark as a block device on DOS 2.x or 3.x
    mov ah, 30h
    int 21h
    cmp al, 3
    ja .exit
    mov word [cs: device_attributes], 0
  .exit:
    pop es
    pop ds
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    retf


GetParams:
    push ds
    push es
    pop ds
  .lp_scandelim:
    lodsb
    call isCRLF
    je .exit
    cmp al, 20h
    ja .lp_scandelim
    dec si
  .lp_skipsp:
    lodsb
    call isCRLF
    je .exit
    cmp al, 20h
    jbe .lp_skipsp
    cmp al, '/'
    je .chkopt
    jmp short .lp_scandelim
  .exit:
    mov al, [cs: optD_err]
    or al, al
    jz .exit_2
    stc
  .exit_2:
    pop ds
    ret
    ;
  .chkopt:
    lodsb
    ;and al, 0E0h	; quick toupper
    cmp al, 'D'
    je .opt_d
    jmp short .lp_scandelim
    ;
  .opt_d:
    lodsb
    cmp al, ':'
    je .opt_d02
    cmp al, '='
    je .opt_d02
    dec si
  .opt_d02:
    mov bx, si
  .opt_dlp:
    lodsb
    cmp al, 20h
    jbe .opt_d03
    call chkDevChar
    jnc .opt_dlp
    mov byte [cs: optD_err], 1
    jmp short .opt_dlp
  .opt_d03:
    dec si
    mov cx, si
    sub cx, bx
    mov ax, 8
    cmp cx, ax
    jb .opt_d04
    mov cx, ax
  .opt_d04:
    sub ax, cx
    push si
    push di
    push ds
    push es
    mov si, bx
    push es
    pop ds
    mov di, devname
    push cs
    pop es
    rep movsb
    mov cx, ax
    mov al, 20h
    rep stosb
    pop es
    pop ds
    pop di
    pop si
    cmp byte [cs: devname], 20h
    jne .optd_end
    mov byte [cs: optD_err], 1
  .optd_end:
    jmp .lp_scandelim

isCRLF:
    cmp al, CR
    je .ret
    cmp al, LF
  .ret:
    ret

chkDevChar:
    cmp al, 'A'
    jb .chkAZ_end
    cmp al, 'Z'
    jbe .ok
  .chkAZ_end:
    cmp al, 'a'
    jb .chkaz_end
    cmp al, 'z'
    jbe .ok
  .chkaz_end:
    cmp al, '0'
    jb .chknum_end
    cmp al, '9'
    jbe .ok
  .chknum_end:
    push cx
    push di
    push es
    mov di, .avail
    mov cx, .avail_end - .avail
    push cs
    pop es
    repne scasb
    pop es
    pop di
    pop cx
    jne .no
  .ok:
    clc
    ret
  .no:
    stc
    ret
  .avail:
    db "!#$%'()-@_`{}~"
  .avail_end:


optD_err:
    db 0
devname:
    db '        '
    db eos

msgOpening1:
    db 'PATACD: Generic ATAPI CD-ROM driver ', eos
devnamePC98:
    db 'CD_101  '
msgPC98:
    db '(PC98) ', eos
devnamePCAT:
    db 'MSCD001 '
msgPCAT:
    db '(PCAT) ', eos
msgOpening2:
    db ' built at '
    db __UTC_DATE__, " ", __UTC_TIME__
    db ' UTC'
    db eos
msgDeviceName:
    db 'CD-ROM device name : '
    db eos

errNoCDROM:
    db 'error: No CD-ROM detected.', eos
errDeviceOption:
    db 'error: invalid option(s).', eos

