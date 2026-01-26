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

section .bss
resb 8192                       ; 8KB of stack memory
stack_space:

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