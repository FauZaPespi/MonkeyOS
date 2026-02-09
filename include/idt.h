#ifndef IDT_H
#define IDT_H

#include "types.h"

// IDT entry (8 bytes each, 256 entries total)
struct idt_entry {
    uint16_t base_low;      // Lower 16 bits of handler address
    uint16_t selector;      // Kernel code segment selector (0x08)
    uint8_t  zero;          // Always 0
    uint8_t  flags;         // Type and attributes
    uint16_t base_high;     // Upper 16 bits of handler address
} __attribute__((packed));

// IDT pointer for LIDT instruction
struct idt_ptr {
    uint16_t limit;         // Size of IDT - 1
    uint32_t base;          // Address of IDT
} __attribute__((packed));

// Flag values
#define IDT_FLAG_PRESENT     0x80   // Present bit
#define IDT_FLAG_RING0       0x00   // Ring 0 (kernel)
#define IDT_FLAG_RING3       0x60   // Ring 3 (user)
#define IDT_FLAG_INTERRUPT   0x0E   // 32-bit interrupt gate
#define IDT_FLAG_TRAP        0x0F   // 32-bit trap gate

// Standard kernel interrupt gate flags
#define IDT_INTERRUPT_GATE   (IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INTERRUPT)

// Function prototypes
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);

// ISR handlers (defined in isr.asm)
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// IRQ handlers (defined in isr.asm)
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

// External assembly function to load IDT
extern void idt_load(struct idt_ptr *ptr);

#endif
