; isr.asm - Interrupt Service Routine stubs
bits 32

section .text

; External C handlers
extern exception_handler
extern irq_handler

; Macro for ISRs that don't push error code
%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0        ; Dummy error code
    push dword %1       ; Interrupt number
    jmp isr_common
%endmacro

; Macro for ISRs that push error code
%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1       ; Interrupt number (error code already on stack)
    jmp isr_common
%endmacro

; Macro for IRQs
%macro IRQ 2
global irq%1
irq%1:
    push dword 0        ; Dummy error code
    push dword %2       ; Interrupt number
    jmp irq_common
%endmacro

; CPU Exceptions (ISR 0-31)
ISR_NOERR 0   ; Division by zero
ISR_NOERR 1   ; Debug
ISR_NOERR 2   ; NMI
ISR_NOERR 3   ; Breakpoint
ISR_NOERR 4   ; Overflow
ISR_NOERR 5   ; Bound Range Exceeded
ISR_NOERR 6   ; Invalid Opcode
ISR_NOERR 7   ; Device Not Available
ISR_ERR   8   ; Double Fault
ISR_NOERR 9   ; Coprocessor Segment Overrun
ISR_ERR   10  ; Invalid TSS
ISR_ERR   11  ; Segment Not Present
ISR_ERR   12  ; Stack-Segment Fault
ISR_ERR   13  ; General Protection Fault
ISR_ERR   14  ; Page Fault
ISR_NOERR 15  ; Reserved
ISR_NOERR 16  ; x87 FPU Error
ISR_ERR   17  ; Alignment Check
ISR_NOERR 18  ; Machine Check
ISR_NOERR 19  ; SIMD FPU Exception
ISR_NOERR 20  ; Virtualization Exception
ISR_NOERR 21  ; Reserved
ISR_NOERR 22  ; Reserved
ISR_NOERR 23  ; Reserved
ISR_NOERR 24  ; Reserved
ISR_NOERR 25  ; Reserved
ISR_NOERR 26  ; Reserved
ISR_NOERR 27  ; Reserved
ISR_NOERR 28  ; Reserved
ISR_NOERR 29  ; Reserved
ISR_NOERR 30  ; Security Exception
ISR_NOERR 31  ; Reserved

; Hardware IRQs (IRQ 0-15 -> INT 32-47)
IRQ 0, 32     ; Timer
IRQ 1, 33     ; Keyboard
IRQ 2, 34     ; Cascade
IRQ 3, 35     ; COM2
IRQ 4, 36     ; COM1
IRQ 5, 37     ; LPT2
IRQ 6, 38     ; Floppy
IRQ 7, 39     ; LPT1 / Spurious
IRQ 8, 40     ; RTC
IRQ 9, 41     ; Free
IRQ 10, 42    ; Free
IRQ 11, 43    ; Free
IRQ 12, 44    ; PS/2 Mouse
IRQ 13, 45    ; Coprocessor
IRQ 14, 46    ; Primary ATA
IRQ 15, 47    ; Secondary ATA

; Common ISR stub - saves state and calls C handler
isr_common:
    pusha               ; Save general registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Push pointer to stack frame
    call exception_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8          ; Remove error code and interrupt number
    iret

; Common IRQ stub
irq_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

; Load IDT
global idt_load
idt_load:
    mov eax, [esp + 4]  ; Get pointer to IDT descriptor
    lidt [eax]          ; Load IDT
    ret
