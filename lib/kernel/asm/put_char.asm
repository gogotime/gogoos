TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0
SELECTOR_DATA  equ (0x0002<<3)+TI_GDT+RPL0

[bits 32]
section .text

global putChar

putChar:
    pushad
    ;jmp $
    mov ax,SELECTOR_VIDEO
    mov gs,ax

;get cursor location
    ;get cursor location high
    mov dx,0x3d4
    mov al,0x0e
    out dx,al
    mov dx,0x3d5
    in  al,dx
    mov ah,al
    ;get cursor location low
    mov dx,0x3d4
    mov al,0x0f
    out dx,al
    mov dx,0x3d5
    in  al,dx

    mov bx,ax
    mov ecx,[esp+36]

    cmp cl,0xd
    je .is_carriage_return
    cmp cl,0xa
    je .is_line_feed
    cmp cl,0x8
    je .is_backspace
    jmp .put_other

.is_backspace:
    dec bx
    shl bx,1

    mov byte [gs:bx],0x20
    inc bx
    mov byte [gs:bx],0x07
    shr bx,1
    jmp .set_cursor

.put_other:
    shl bx,1

    mov byte [gs:bx],cl
    inc bx
    mov byte [gs:bx],0x07
    shr bx,1
    inc bx
    cmp bx,2000
    jb .set_cursor

.is_carriage_return:
.is_line_feed:
    xor dx,dx
    mov ax,bx
    mov si,80
    div si

    sub bx,dx
    add bx,80
    cmp bx,2000
    jb .set_cursor

.roll_screen:
    mov ecx,960
    mov esi,0xc00b80a0
    mov edi,0xc00b8000
    cld
    rep movsd

    mov ebx,3840
    mov ecx,40
.clear_the_last_line:
    mov dword [gs:ebx],0x07200720
    add ebx,4
    loop .clear_the_last_line
    mov ebx,1920

.set_cursor:
;set cursor location
    ;set cursor location high
    mov dx,0x3d4
    mov al,0x0e
    out dx,al
    mov dx,0x3d5
    mov al,bh
    out dx,al
    ;set cursor location low
    mov dx,0x3d4
    mov al,0x0f
    out dx,al
    mov dx,0x3d5
    mov al,bl
    out dx,al

.put_char_done:
    popad
    ret