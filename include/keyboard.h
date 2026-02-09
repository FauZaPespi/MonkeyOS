#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

// PS/2 keyboard ports
#define KB_DATA_PORT    0x60    // Read: scan code, Write: commands
#define KB_STATUS_PORT  0x64    // Read: status, Write: commands
#define KB_COMMAND_PORT 0x64

// Status register bits
#define KB_STATUS_OUTPUT_FULL   0x01    // Output buffer full (can read)
#define KB_STATUS_INPUT_FULL    0x02    // Input buffer full (don't write)

// Keyboard buffer size
#define KB_BUFFER_SIZE  256

// Special key codes (values > 127 to distinguish from ASCII)
#define KEY_UP          0x80
#define KEY_DOWN        0x81
#define KEY_LEFT        0x82
#define KEY_RIGHT       0x83
#define KEY_HOME        0x84
#define KEY_END         0x85
#define KEY_PGUP        0x86
#define KEY_PGDN        0x87
#define KEY_DELETE      0x88
#define KEY_INSERT      0x89
#define KEY_F1          0x8A
#define KEY_F2          0x8B
#define KEY_F3          0x8C
#define KEY_F4          0x8D
#define KEY_F5          0x8E
#define KEY_F6          0x8F
#define KEY_F7          0x90
#define KEY_F8          0x91
#define KEY_F9          0x92
#define KEY_F10         0x93
#define KEY_F11         0x94
#define KEY_F12         0x95

// Keyboard state
struct keyboard_state {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;

    // Circular buffer for input
    char buffer[KB_BUFFER_SIZE];
    uint16_t buffer_head;   // Write position
    uint16_t buffer_tail;   // Read position
    uint16_t buffer_count;  // Characters in buffer
};

// Function prototypes
void keyboard_init(void);
void keyboard_handler(void);          // Called by ISR
char keyboard_getchar(void);          // Blocking read
bool keyboard_has_input(void);        // Check if data available
char keyboard_getchar_nonblock(void); // Non-blocking read
void keyboard_set_echo(bool enabled); // Enable/disable auto-echo
bool keyboard_ctrl_pressed(void);     // Check if Ctrl is held

#endif
