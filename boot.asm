; boot.asm
bits 32                         ; We are in 32-bit protected mode
section .text
        align 4
        dd 0x1BADB002           ; Magic number for Multiboot
        dd 0x00                 ; Flags
        dd - (0x1BADB002 + 0x00); Checksum

global _start
extern kmain                    ; This is our C function

_start:
  cli                           ; Clear interrupts
  mov esp, stack_space          ; Set up a stack for C to use
  call kmain                    ; Jump to C!
  hlt                           ; Halt if C returns

; Port I/O functions (must be in .text section)
global inb
inb:
    mov edx, [esp + 4] ; Get the port number from the stack
    in al, dx          ; Read from port DX into AL
    ret

global outb
outb:
    mov edx, [esp + 4] ; Get port number
    mov eax, [esp + 8] ; Get data
    out dx, al         ; Write AL to port DX
    ret

; 16-bit port I/O (needed for ATA)
global inw
inw:
    mov edx, [esp + 4]
    in ax, dx
    ret

global outw
outw:
    mov edx, [esp + 4]
    mov eax, [esp + 8]
    out dx, ax
    ret

; Block I/O (read multiple words from port)
global insw
insw:
    push edi
    mov edx, [esp + 8]      ; port
    mov edi, [esp + 12]     ; destination address
    mov ecx, [esp + 16]     ; count (words)
    rep insw
    pop edi
    ret

; Block I/O (write multiple words to port)
global outsw
outsw:
    push esi
    mov edx, [esp + 8]      ; port
    mov esi, [esp + 12]     ; source address
    mov ecx, [esp + 16]     ; count (words)
    rep outsw
    pop esi
    ret

section .bss
resb 8192                       ; 8KB of stack memory
stack_space:
