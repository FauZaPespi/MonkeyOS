// kernel.c - MonkeyOS Kernel
#include "types.h"
#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "ata.h"
#include "fs.h"
#include "shell.h"

// Exception names for debugging
static const char *exception_names[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Security Exception", "Reserved"
};

// C exception handler (called from isr.asm)
void exception_handler(uint32_t *regs) {
    // Stack frame layout after pusha and segment pushes:
    // [esp+0]  = gs
    // [esp+4]  = fs
    // [esp+8]  = es
    // [esp+12] = ds
    // [esp+16] = edi (from pusha)
    // [esp+20] = esi
    // [esp+24] = ebp
    // [esp+28] = esp (original)
    // [esp+32] = ebx
    // [esp+36] = edx
    // [esp+40] = ecx
    // [esp+44] = eax
    // [esp+48] = int_no
    // [esp+52] = err_code
    // [esp+56] = eip
    // [esp+60] = cs
    // [esp+64] = eflags

    uint32_t int_no = regs[12];    // Interrupt number
    uint32_t err_code = regs[13];  // Error code

    vga_set_color(VGA_WHITE, VGA_RED);
    vga_puts("\n\n !!! KERNEL PANIC !!! \n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    vga_puts("Exception: ");
    if (int_no < 32) {
        vga_puts(exception_names[int_no]);
    } else {
        vga_puts("Unknown");
    }
    vga_puts(" (INT ");
    vga_put_dec(int_no);
    vga_puts(")\n");

    vga_puts("Error code: ");
    vga_put_hex(err_code);
    vga_puts("\n");

    // Halt the system
    vga_puts("\nSystem halted.");
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}

// C IRQ handler (called from isr.asm)
void irq_handler(uint32_t *regs) {
    uint32_t int_no = regs[12];
    uint8_t irq = int_no - 32;

    switch (irq) {
        case 0:  // Timer (IRQ0)
            // Timer tick handling (not implemented yet)
            break;
        case 1:  // Keyboard (IRQ1)
            keyboard_handler();
            break;
        case 14: // Primary ATA (IRQ14)
            // ATA interrupt handling (for DMA mode - not used in PIO)
            break;
        case 15: // Secondary ATA (IRQ15)
            break;
    }

    // Send End of Interrupt
    pic_send_eoi(irq);
}

void kmain(void) {
    // Initialize VGA display
    vga_init();
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("MonkeyOS v0.2\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("Initializing hardware...\n\n");

    // Initialize PIC (Programmable Interrupt Controller)
    vga_puts("[*] PIC: Remapping interrupts to 0x20-0x2F\n");
    pic_init();

    // Initialize IDT (Interrupt Descriptor Table)
    vga_puts("[*] IDT: Setting up interrupt handlers\n");
    idt_init();

    // Enable interrupts
    vga_puts("[*] Enabling interrupts\n");
    __asm__ volatile("sti");

    // Initialize keyboard
    vga_puts("[*] Keyboard: Initializing PS/2 driver\n");
    keyboard_init();

    // Initialize ATA disk controller
    vga_puts("[*] ");
    ata_init();

    // Initialize filesystem
    vga_puts("[*] Filesystem: ");
    fs_init();

    // System ready - start shell
    vga_puts("\n");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("=== Starting Shell ===\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    // Run the shell (main user interface)
    shell_run();

    // If shell exits, halt
    vga_puts("\nShell exited. System halted.\n");
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}
