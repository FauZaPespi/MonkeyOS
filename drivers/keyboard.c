#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "vga.h"

static struct keyboard_state kb_state;
static bool echo_enabled = TRUE;
static bool extended_scancode = FALSE;

// US QWERTY scancode set 1 to ASCII (normal)
static const char scancode_to_ascii[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0,   // 55-58: *, Alt, Space, Caps
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // F1-F10
    0, 0,  // Num Lock, Scroll Lock
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,  // 84-88
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 89-98
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 99-108
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 109-118
    0, 0, 0, 0, 0, 0, 0, 0, 0      // 119-127
};

// Shifted scancode mappings
static const char scancode_to_ascii_shift[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_init(void) {
    kb_state.shift_pressed = FALSE;
    kb_state.ctrl_pressed = FALSE;
    kb_state.alt_pressed = FALSE;
    kb_state.caps_lock = FALSE;
    kb_state.buffer_head = 0;
    kb_state.buffer_tail = 0;
    kb_state.buffer_count = 0;

    // Clear keyboard buffer by reading any pending data
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) {
        inb(KB_DATA_PORT);
    }

    // Enable keyboard IRQ (IRQ1)
    pic_clear_mask(1);
}

static void buffer_put(char c) {
    if (kb_state.buffer_count < KB_BUFFER_SIZE) {
        kb_state.buffer[kb_state.buffer_head] = c;
        kb_state.buffer_head = (kb_state.buffer_head + 1) % KB_BUFFER_SIZE;
        kb_state.buffer_count++;
    }
}

static char buffer_get(void) {
    if (kb_state.buffer_count == 0) {
        return 0;
    }
    char c = kb_state.buffer[kb_state.buffer_tail];
    kb_state.buffer_tail = (kb_state.buffer_tail + 1) % KB_BUFFER_SIZE;
    kb_state.buffer_count--;
    return c;
}

void keyboard_handler(void) {
    uint8_t scancode = inb(KB_DATA_PORT);

    // Handle extended scancode prefix
    if (scancode == 0xE0) {
        extended_scancode = TRUE;
        return;
    }

    // Check if key release (bit 7 set)
    bool released = (scancode & 0x80) != 0;
    scancode &= 0x7F;  // Clear release bit

    // Handle extended scancodes (arrow keys, etc.)
    if (extended_scancode) {
        extended_scancode = FALSE;
        if (!released) {
            char special = 0;
            switch (scancode) {
                case 0x48: special = KEY_UP; break;
                case 0x50: special = KEY_DOWN; break;
                case 0x4B: special = KEY_LEFT; break;
                case 0x4D: special = KEY_RIGHT; break;
                case 0x47: special = KEY_HOME; break;
                case 0x4F: special = KEY_END; break;
                case 0x49: special = KEY_PGUP; break;
                case 0x51: special = KEY_PGDN; break;
                case 0x53: special = KEY_DELETE; break;
                case 0x52: special = KEY_INSERT; break;
                case 0x1D: kb_state.ctrl_pressed = TRUE; return;  // Right Ctrl
                case 0x38: kb_state.alt_pressed = TRUE; return;   // Right Alt
            }
            if (special != 0) {
                buffer_put(special);
            }
        } else {
            // Handle extended key release
            switch (scancode) {
                case 0x1D: kb_state.ctrl_pressed = FALSE; break;
                case 0x38: kb_state.alt_pressed = FALSE; break;
            }
        }
        return;
    }

    // Handle modifier keys
    switch (scancode) {
        case 0x2A:  // Left Shift
        case 0x36:  // Right Shift
            kb_state.shift_pressed = !released;
            return;
        case 0x1D:  // Ctrl
            kb_state.ctrl_pressed = !released;
            return;
        case 0x38:  // Alt
            kb_state.alt_pressed = !released;
            return;
        case 0x3A:  // Caps Lock (toggle on press only)
            if (!released) {
                kb_state.caps_lock = !kb_state.caps_lock;
            }
            return;
    }

    // Only process key presses, not releases
    if (released) {
        return;
    }

    // Get ASCII character
    char c;
    if (kb_state.shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }

    // Apply caps lock to letters
    if (kb_state.caps_lock) {
        if (c >= 'a' && c <= 'z') {
            c -= 32;  // To uppercase
        } else if (c >= 'A' && c <= 'Z') {
            c += 32;  // To lowercase (shift + caps = lowercase)
        }
    }

    // Handle Ctrl+key combinations
    if (kb_state.ctrl_pressed && c >= 'a' && c <= 'z') {
        c = c - 'a' + 1;  // Ctrl+A = 1, Ctrl+B = 2, etc.
    }

    if (c != 0) {
        buffer_put(c);
        // Echo to screen if enabled
        if (echo_enabled) {
            vga_putchar(c);
        }
    }
}

bool keyboard_has_input(void) {
    return kb_state.buffer_count > 0;
}

char keyboard_getchar(void) {
    // Blocking read - wait for input
    while (!keyboard_has_input()) {
        __asm__ volatile("hlt");  // Wait for interrupt
    }
    return buffer_get();
}

char keyboard_getchar_nonblock(void) {
    return buffer_get();  // Returns 0 if empty
}

void keyboard_set_echo(bool enabled) {
    echo_enabled = enabled;
}

bool keyboard_ctrl_pressed(void) {
    return kb_state.ctrl_pressed;
}
