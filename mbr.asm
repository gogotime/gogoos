;Main Boot Loader
%include "boot.inc"
;-------------------------------------
SECTION MBR vstart=0x7c00
;-------------------------------------
;Init Register
;-------------------------------------
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800
    mov gs,ax
    
;-------------------------------------
;Clear Screen
;-------------------------------------    
    mov ax,0x0600
    mov bx,0x0700
    mov cx,0
    mov dx,0x184f
    int 0x10
    
;-------------------------------------
;print "LDMBR"
;-------------------------------------   
    mov byte [gs:0x00],'L'
    mov byte [gs:0x01],0xA4
    
    mov byte [gs:0x02],'D'
    mov byte [gs:0x03],0xA4
    
    mov byte [gs:0x04],'M'
    mov byte [gs:0x05],0xA4
    
    mov byte [gs:0x06],'B'
    mov byte [gs:0x07],0xA4
    
    mov byte [gs:0x08],'R'
    mov byte [gs:0x09],0xA4

;-------------------------------------
;call read_disk
;-------------------------------------    
    mov eax,LOADER_START_SECTOR 
    mov bx,LOADER_BASE_ADDR
    mov cx,4
    call read_disk_16
    
;-------------------------------------
;jump core loader
;-------------------------------------   
    jmp LOADER_BASE_ADDR

;-------------------------------------
;function read_disk
;-------------------------------------   
read_disk_16:
;1.set sector number
    mov esi,eax
    mov di,cx
    mov dx,0x1f2
    mov al,cl
    out dx,al              
    mov eax,esi            

;2.set LBA address
    mov dx,0x1f3
    out dx,al
    
    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al
    
    shr eax,cl
    mov dx,0x1f5
    out dx,al    
    
    shr eax,cl
    and al,0x0f
    or  al,0xe0
    mov dx,0x1f6
    out dx,al
    
;3.set command READ
    mov dx,0x1f7
    mov al,0x20
    out dx,al
    
;4.read status
.not_ready:
    nop
    in al,dx
    and al,0x88
    cmp al,0x08
    jnz .not_ready

;5.read data
    mov ax,di
    mov dx,256
    mul dx
    mov cx,ax
    mov dx,0x1f0
.go_on_read:
    in ax,dx
    mov [bx],ax
    add bx,2
    loop .go_on_read
    ret
    
    
times 510-($-$$) db 0
db 0x55,0xaa
