#include "vga.h"
#include "io.h"

#define VGA_ADDRESS     0xB8000
#define VGA_CTRL_PORT   0x3D4
#define VGA_DATA_PORT   0x3D5

static char *video_memory = (char *)VGA_ADDRESS;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t text_color = 0x07;  // Light grey on black

void vga_init(void) {
    vga_clear();
    vga_update_cursor();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        video_memory[i] = ' ';
        video_memory[i + 1] = text_color;
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
}

void vga_scroll(void) {
    // Move all lines up by one
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH * 2; i++) {
        video_memory[i] = video_memory[i + VGA_WIDTH * 2];
    }
    // Clear last line
    int start = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
    for (int i = 0; i < VGA_WIDTH * 2; i += 2) {
        video_memory[start + i] = ' ';
        video_memory[start + i + 1] = text_color;
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
            video_memory[offset] = ' ';
            video_memory[offset + 1] = text_color;
        }
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;  // Align to 8
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    } else {
        int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        video_memory[offset] = c;
        video_memory[offset + 1] = text_color;
        cursor_x++;
    }

    // Handle line wrap
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    // Handle scroll
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }

    vga_update_cursor();
}

void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(VGA_CTRL_PORT, 0x0F);
    outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_PORT, 0x0E);
    outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    text_color = (bg << 4) | (fg & 0x0F);
}

void vga_put_hex(uint32_t value) {
    const char hex[] = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        vga_putchar(hex[(value >> i) & 0xF]);
    }
}

void vga_put_dec(uint32_t value) {
    if (value == 0) {
        vga_putchar('0');
        return;
    }

    char buf[12];
    int i = 0;
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (--i >= 0) {
        vga_putchar(buf[i]);
    }
}

void vga_set_cursor(uint8_t x, uint8_t y) {
    if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;
    cursor_x = x;
    cursor_y = y;
    vga_update_cursor();
}

void vga_get_cursor(uint8_t *x, uint8_t *y) {
    *x = cursor_x;
    *y = cursor_y;
}

void vga_clear_line(uint8_t y) {
    if (y >= VGA_HEIGHT) return;
    int start = y * VGA_WIDTH * 2;
    for (int i = 0; i < VGA_WIDTH * 2; i += 2) {
        video_memory[start + i] = ' ';
        video_memory[start + i + 1] = text_color;
    }
}

void vga_putchar_at(char c, uint8_t x, uint8_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    int offset = (y * VGA_WIDTH + x) * 2;
    video_memory[offset] = c;
    video_memory[offset + 1] = text_color;
}

void vga_putchar_at_color(char c, uint8_t x, uint8_t y, uint8_t color) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    int offset = (y * VGA_WIDTH + x) * 2;
    video_memory[offset] = c;
    video_memory[offset + 1] = color;
}

void vga_puts_at(const char *str, uint8_t x, uint8_t y) {
    while (*str && x < VGA_WIDTH) {
        vga_putchar_at(*str++, x++, y);
    }
}

uint8_t vga_get_color(void) {
    return text_color;
}
