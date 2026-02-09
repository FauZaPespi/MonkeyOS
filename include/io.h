#ifndef IO_H
#define IO_H

#include "types.h"

// Byte I/O (implemented in boot.asm)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// Word I/O (implemented in boot.asm)
extern uint16_t inw(uint16_t port);
extern void outw(uint16_t port, uint16_t data);

// Block I/O (implemented in boot.asm)
extern void insw(uint16_t port, void *addr, uint32_t count);
extern void outsw(uint16_t port, const void *addr, uint32_t count);

// I/O delay (for slow devices like PIC)
static inline void io_wait(void) {
    outb(0x80, 0);  // Port 0x80 is used for POST codes, safe to write
}

#endif
