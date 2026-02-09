#ifndef EDITOR_H
#define EDITOR_H

#include "types.h"

#define EDITOR_MAX_LINES    50
#define EDITOR_MAX_COLS     80
#define EDITOR_VISIBLE_LINES 23  // Lines 0-22, status on 23-24

struct editor_state {
    char     filename[32];
    uint32_t file_inode;
    bool     is_new_file;
    bool     modified;

    // Text buffer
    char     lines[EDITOR_MAX_LINES][EDITOR_MAX_COLS];
    uint16_t line_count;
    uint16_t line_lengths[EDITOR_MAX_LINES];

    // Cursor position
    uint16_t cursor_row;
    uint16_t cursor_col;

    // View position (for scrolling)
    uint16_t view_top;

    // Status message
    char     status_msg[80];

    // Running flag
    bool     running;
};

// Editor functions
void editor_run(const char *filename);

#endif
