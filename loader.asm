bits 16
%include "boot.inc"
[bits 16]
LOADER_STACK_TOP equ LOADER_BASE_ADDR

section loader vstart=LOADER_BASE_ADDR

    jmp loader_start
    align   4
    GDT_BASE:    dd 0x00000000
                 dd 0x00000000
    CODE_DESC:   dd 0x0000ffff
                 dd DESC_CODE_HIGH_4_BYTE
    DATA_DESC:   dd 0x0000ffff
                 dd DESC_DATA_HIGH_4_BYTE
    VIDEO_DESC:  dd 0x80000007
                 dd DESC_VIDEO_HIGH_4_BYTE
    GDT_SIZE  equ $ - GDT_BASE
    GDT_LIMIT equ GDT_SIZE -1

    times 60 dq 0


    SELECTOR_CODE  equ (0x0001<<3) + TI_GDT + RPL0
    SELECTOR_DATA  equ (0x0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE

    align 4
    total_mem_bytes dd 0
    ards_buf times 256 db 0
    ards_nr dw 0
loader_start:
;- int 15h eax=0000e820h edx=534d4150h ('SMAP') Get Memory -
    xor ebx,ebx
    mov edx,0x534d4150
    mov di,ards_buf
.e820_get_mem_loop:
    mov eax,0x0000e820
    mov ecx,20
    int 0x15
    jc .failed_to_get_mem
    add di,cx
    inc word [ards_nr]
    cmp bx,0
    jnz .e820_get_mem_loop

    mov cx,[ards_nr]
    mov ebx,ards_buf
    xor edx,edx
.find_max_mem_area:
    mov eax,[ebx]
    add eax,[ebx+8]
    add ebx,20
    cmp edx,eax
    jge .next_ards
    mov edx,eax
.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

.failed_to_get_mem:
    mov ax,0xb800
    mov gs,ax

    mov byte [gs:160],'F'
    mov byte [gs:161],0xA4

    mov byte [gs:162],'G'
    mov byte [gs:163],0xA4

    mov byte [gs:164],'E'
    mov byte [gs:165],0xA4

    mov byte [gs:166],'T'
    mov byte [gs:167],0xA4

    mov byte [gs:168],'M'
    mov byte [gs:169],0xA4

    mov byte [gs:170],'E'
    mov byte [gs:171],0xA4

    mov byte [gs:172],'M'
    mov byte [gs:173],0xA4

.mem_get_ok:
    mov [total_mem_bytes],edx

;-----------open A20----------------
    in al,0x92
    or al,00000010b
    out 0x92,al

;----------load gdt-----------------
    lgdt [gdt_ptr]

;------set zeroth bit in cr0 1------
    mov eax,cr0
    or eax,0x00000001
    mov cr0,eax
;-----refresh pipeline--------------
    jmp dword SELECTOR_CODE:p_mode_start

;-----protect mode------------------
[bits 32]
p_mode_start:
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp:LOADER_STACK_TOP
    mov ax,SELECTOR_VIDEO
    mov gs,ax

    mov byte [gs:160],'P'
    mov byte [gs:161],0xA4

    mov byte [gs:162],'R'
    mov byte [gs:163],0xA4

    mov byte [gs:164],'O'
    mov byte [gs:165],0xA4

    mov byte [gs:166],'T'
    mov byte [gs:167],0xA4

    mov byte [gs:168],'E'
    mov byte [gs:169],0xA4

    mov byte [gs:170],'C'
    mov byte [gs:171],0xA4

    mov byte [gs:172],'T'
    mov byte [gs:173],0xA4

;--------- load kernel --------------
    mov eax,KERNEL_START_SECTOR
    mov ebx,KERNEL_BIN_BASE_ADDR
    mov cx,200
    call read_disk_32

;---------- use page ----------------
    call setup_page

    sgdt [gdt_ptr];save gdt


    mov ebx,[gdt_ptr+2]
    or dword [ebx+0x18+4],0xc0000000
    add dword [gdt_ptr+2],0xc0000000
    add esp,0xc0000000

    mov eax,PAGE_DIR_TABLE_POS
    mov cr3,eax

    mov eax,cr0
    or  eax,0x80000000
    mov cr0,eax

    lgdt [gdt_ptr]

    mov byte [gs:320],'V'
    mov byte [gs:321],0xA4

    ;jmp $
    ;jmp SELECTOR_CODE:enter_kernel

enter_kernel:
    call kernel_init
    mov esp,0xc009f000
    jmp KERNEL_ENTRY_POINT

;function kernel_init
kernel_init:
    xor eax,eax
    xor ebx,ebx
    xor ecx,ecx
    xor edx,edx

    mov dx,[KERNEL_BIN_BASE_ADDR + 42]
    mov ebx,[KERNEL_BIN_BASE_ADDR + 28]
    add ebx,KERNEL_BIN_BASE_ADDR
    mov cx,[KERNEL_BIN_BASE_ADDR + 44]

.each_segment:
    cmp dword [ebx + 0],PT_NULL
    je .PTNULL
    cmp dword [ebx + 8],0xc0000000
    jb .BADADDR
    push dword [ebx + 16] ;push size
    mov eax,[ebx + 4]
    add eax,KERNEL_BIN_BASE_ADDR
    push eax ;push src
    push dword [ebx+8] ;push dst
    call mem_cpy
    add esp,12

.PTNULL:
.BADADDR:
    add ebx,edx
    loop .each_segment
    ret
;function mem_cpy(dst,src,size)
mem_cpy:
    cld
    push ebp
    mov ebp,esp
    push ecx

    mov edi,[ebp+8]
    mov esi,[ebp+12]
    mov ecx,[ebp+16]


    rep movsb

    pop ecx
    pop ebp
    ret

;function read_disk_32
read_disk_32:
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
    mov [ebx],ax
    add ebx,2
    loop .go_on_read
    ret

;function setup_page
setup_page:
    mov ecx,4096
    mov esi,0

.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS+esi],0
    inc esi
    loop .clear_page_dir

;create Page Directory Entry
.create_pde:
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000
    mov ebx,eax
    or eax,PG_US_U|PG_RW_W|PG_P
    mov [PAGE_DIR_TABLE_POS+0x0],eax
    mov [PAGE_DIR_TABLE_POS+0xc00],eax
    sub eax,0x1000
    mov [PAGE_DIR_TABLE_POS+4092],eax

;create Page Table Entry
    mov ecx,256
    mov esi,0
    mov edx,PG_US_U|PG_RW_W|PG_P
.create_pte:
    mov [ebx+esi*4],edx
    add edx,4096
    inc esi
    loop .create_pte

;create Kernel PDE
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000
    or eax,PG_US_U|PG_RW_W|PG_P
    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254
    mov esi,769
.create_kernel_pde:
    mov [ebx+esi*4],eax
    inc esi
    add eax,0x1000
    loop .create_kernel_pde
    ret



times 2048-($-$$) db 0