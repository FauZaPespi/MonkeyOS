#include "editor.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "fs.h"

static struct editor_state editor;

// Status bar colors
#define STATUS_BAR_COLOR ((VGA_BLACK << 4) | VGA_WHITE)
#define STATUS_BAR_ROW   23
#define MESSAGE_BAR_ROW  24

static void editor_init(const char *filename) {
    mem_set(&editor, 0, sizeof(struct editor_state));
    str_ncpy(editor.filename, filename, 31);
    editor.line_count = 1;
    editor.cursor_row = 0;
    editor.cursor_col = 0;
    editor.view_top = 0;
    editor.modified = FALSE;
    editor.running = TRUE;

    // Initialize first line as empty
    editor.lines[0][0] = '\0';
    editor.line_lengths[0] = 0;
}

static void editor_load_file(void) {
    uint32_t inode_num;

    if (!fs_open(editor.filename, &inode_num)) {
        editor.is_new_file = TRUE;
        str_cpy(editor.status_msg, "New file");
        return;
    }

    editor.is_new_file = FALSE;
    editor.file_inode = inode_num;

    // Read file content
    static uint8_t file_buf[EDITOR_MAX_LINES * EDITOR_MAX_COLS];
    uint32_t size = 0;

    if (!fs_read(inode_num, file_buf, &size)) {
        str_cpy(editor.status_msg, "Error reading file");
        return;
    }

    // Parse into lines
    editor.line_count = 0;
    uint32_t col = 0;

    for (uint32_t i = 0; i < size && editor.line_count < EDITOR_MAX_LINES; i++) {
        char c = file_buf[i];

        if (c == '\n' || col >= EDITOR_MAX_COLS - 1) {
            editor.lines[editor.line_count][col] = '\0';
            editor.line_lengths[editor.line_count] = col;
            editor.line_count++;
            col = 0;
        } else if (c >= 32 || c == '\t') {
            editor.lines[editor.line_count][col++] = c;
        }
    }

    // Handle last line if not ending with newline
    if (col > 0 || editor.line_count == 0) {
        editor.lines[editor.line_count][col] = '\0';
        editor.line_lengths[editor.line_count] = col;
        editor.line_count++;
    }

    if (editor.line_count == 0) {
        editor.line_count = 1;
        editor.lines[0][0] = '\0';
        editor.line_lengths[0] = 0;
    }

    str_cpy(editor.status_msg, "File loaded");
}

static void editor_save_file(void) {
    // Create file if new
    if (editor.is_new_file) {
        if (!fs_create(editor.filename)) {
            str_cpy(editor.status_msg, "Error creating file");
            return;
        }
        if (!fs_open(editor.filename, &editor.file_inode)) {
            str_cpy(editor.status_msg, "Error opening file");
            return;
        }
        editor.is_new_file = FALSE;
    }

    // Build file content
    static uint8_t file_buf[EDITOR_MAX_LINES * EDITOR_MAX_COLS];
    uint32_t pos = 0;

    for (uint16_t i = 0; i < editor.line_count; i++) {
        uint16_t len = editor.line_lengths[i];
        mem_cpy(file_buf + pos, editor.lines[i], len);
        pos += len;
        file_buf[pos++] = '\n';
    }

    // Write to disk
    if (fs_write(editor.file_inode, file_buf, pos)) {
        editor.modified = FALSE;
        str_cpy(editor.status_msg, "File saved");
    } else {
        str_cpy(editor.status_msg, "Error saving file");
    }
}

static void editor_draw_line(uint8_t screen_row, uint16_t file_line) {
    vga_clear_line(screen_row);

    if (file_line < editor.line_count) {
        for (int i = 0; i < editor.line_lengths[file_line] && i < VGA_WIDTH; i++) {
            vga_putchar_at(editor.lines[file_line][i], i, screen_row);
        }
    } else {
        // Empty line indicator
        vga_set_color(VGA_DARK_GREY, VGA_BLACK);
        vga_putchar_at('~', 0, screen_row);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

static void editor_draw_status_bar(void) {
    // Save current color
    uint8_t old_color = vga_get_color();

    // Draw status bar background
    for (int i = 0; i < VGA_WIDTH; i++) {
        vga_putchar_at_color(' ', i, STATUS_BAR_ROW, STATUS_BAR_COLOR);
    }

    // Left side: keybindings
    const char *help = " ^S Save  ^X Exit ";
    for (int i = 0; help[i] && i < 20; i++) {
        vga_putchar_at_color(help[i], i, STATUS_BAR_ROW, STATUS_BAR_COLOR);
    }

    // Middle: filename and modified indicator
    char info[40];
    int len = 0;

    if (editor.modified) {
        info[len++] = '*';
    }

    int name_len = str_len(editor.filename);
    if (name_len > 20) name_len = 20;
    mem_cpy(info + len, editor.filename, name_len);
    len += name_len;
    info[len] = '\0';

    int info_start = (VGA_WIDTH - len) / 2;
    for (int i = 0; info[i]; i++) {
        vga_putchar_at_color(info[i], info_start + i, STATUS_BAR_ROW, STATUS_BAR_COLOR);
    }

    // Right side: position
    char pos_buf[20];
    int pos_len = 0;
    pos_buf[pos_len++] = 'L';
    pos_buf[pos_len++] = 'n';
    pos_buf[pos_len++] = ':';
    pos_len += uint_to_str(editor.cursor_row + 1, pos_buf + pos_len);
    pos_buf[pos_len++] = ' ';
    pos_buf[pos_len++] = 'C';
    pos_buf[pos_len++] = 'o';
    pos_buf[pos_len++] = 'l';
    pos_buf[pos_len++] = ':';
    pos_len += uint_to_str(editor.cursor_col + 1, pos_buf + pos_len);
    pos_buf[pos_len++] = ' ';
    pos_buf[pos_len] = '\0';

    int pos_start = VGA_WIDTH - pos_len;
    for (int i = 0; pos_buf[i]; i++) {
        vga_putchar_at_color(pos_buf[i], pos_start + i, STATUS_BAR_ROW, STATUS_BAR_COLOR);
    }

    // Restore color
    vga_set_color(old_color & 0x0F, (old_color >> 4) & 0x0F);
}

static void editor_draw_message_bar(void) {
    vga_clear_line(MESSAGE_BAR_ROW);

    if (editor.status_msg[0]) {
        vga_set_color(VGA_YELLOW, VGA_BLACK);
        vga_puts_at(editor.status_msg, 0, MESSAGE_BAR_ROW);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

static void editor_refresh_screen(void) {
    // Adjust view if cursor is off-screen
    if (editor.cursor_row < editor.view_top) {
        editor.view_top = editor.cursor_row;
    }
    if (editor.cursor_row >= editor.view_top + EDITOR_VISIBLE_LINES) {
        editor.view_top = editor.cursor_row - EDITOR_VISIBLE_LINES + 1;
    }

    // Draw file content
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    for (uint8_t y = 0; y < EDITOR_VISIBLE_LINES; y++) {
        editor_draw_line(y, editor.view_top + y);
    }

    // Draw status bars
    editor_draw_status_bar();
    editor_draw_message_bar();

    // Position cursor
    uint8_t screen_row = editor.cursor_row - editor.view_top;
    vga_set_cursor(editor.cursor_col, screen_row);
}

static void editor_insert_char(char c) {
    uint16_t *len = &editor.line_lengths[editor.cursor_row];

    if (*len >= EDITOR_MAX_COLS - 1) return;

    // Shift characters right
    for (int i = *len; i > editor.cursor_col; i--) {
        editor.lines[editor.cursor_row][i] = editor.lines[editor.cursor_row][i - 1];
    }

    // Insert character
    editor.lines[editor.cursor_row][editor.cursor_col] = c;
    (*len)++;
    editor.lines[editor.cursor_row][*len] = '\0';
    editor.cursor_col++;
    editor.modified = TRUE;
}

static void editor_delete_char(void) {
    if (editor.cursor_col > 0) {
        // Delete character before cursor
        uint16_t *len = &editor.line_lengths[editor.cursor_row];

        for (int i = editor.cursor_col - 1; i < *len; i++) {
            editor.lines[editor.cursor_row][i] = editor.lines[editor.cursor_row][i + 1];
        }

        (*len)--;
        editor.cursor_col--;
        editor.modified = TRUE;
    } else if (editor.cursor_row > 0) {
        // Join with previous line
        uint16_t prev_len = editor.line_lengths[editor.cursor_row - 1];
        uint16_t curr_len = editor.line_lengths[editor.cursor_row];

        if (prev_len + curr_len < EDITOR_MAX_COLS - 1) {
            // Append current line to previous
            str_cat(editor.lines[editor.cursor_row - 1], editor.lines[editor.cursor_row]);
            editor.line_lengths[editor.cursor_row - 1] = prev_len + curr_len;

            // Shift lines up
            for (int i = editor.cursor_row; i < editor.line_count - 1; i++) {
                str_cpy(editor.lines[i], editor.lines[i + 1]);
                editor.line_lengths[i] = editor.line_lengths[i + 1];
            }

            editor.line_count--;
            editor.cursor_row--;
            editor.cursor_col = prev_len;
            editor.modified = TRUE;
        }
    }
}

static void editor_insert_newline(void) {
    if (editor.line_count >= EDITOR_MAX_LINES) return;

    // Shift lines down
    for (int i = editor.line_count; i > editor.cursor_row + 1; i--) {
        str_cpy(editor.lines[i], editor.lines[i - 1]);
        editor.line_lengths[i] = editor.line_lengths[i - 1];
    }

    // Split current line
    uint16_t split_pos = editor.cursor_col;
    uint16_t old_len = editor.line_lengths[editor.cursor_row];

    // Copy rest of line to new line
    str_cpy(editor.lines[editor.cursor_row + 1], editor.lines[editor.cursor_row] + split_pos);
    editor.line_lengths[editor.cursor_row + 1] = old_len - split_pos;

    // Truncate current line
    editor.lines[editor.cursor_row][split_pos] = '\0';
    editor.line_lengths[editor.cursor_row] = split_pos;

    editor.line_count++;
    editor.cursor_row++;
    editor.cursor_col = 0;
    editor.modified = TRUE;
}

static void editor_move_cursor(uint8_t key) {
    switch (key) {
        case KEY_UP:
            if (editor.cursor_row > 0) {
                editor.cursor_row--;
                if (editor.cursor_col > editor.line_lengths[editor.cursor_row]) {
                    editor.cursor_col = editor.line_lengths[editor.cursor_row];
                }
            }
            break;

        case KEY_DOWN:
            if (editor.cursor_row < editor.line_count - 1) {
                editor.cursor_row++;
                if (editor.cursor_col > editor.line_lengths[editor.cursor_row]) {
                    editor.cursor_col = editor.line_lengths[editor.cursor_row];
                }
            }
            break;

        case KEY_LEFT:
            if (editor.cursor_col > 0) {
                editor.cursor_col--;
            } else if (editor.cursor_row > 0) {
                editor.cursor_row--;
                editor.cursor_col = editor.line_lengths[editor.cursor_row];
            }
            break;

        case KEY_RIGHT:
            if (editor.cursor_col < editor.line_lengths[editor.cursor_row]) {
                editor.cursor_col++;
            } else if (editor.cursor_row < editor.line_count - 1) {
                editor.cursor_row++;
                editor.cursor_col = 0;
            }
            break;

        case KEY_HOME:
            editor.cursor_col = 0;
            break;

        case KEY_END:
            editor.cursor_col = editor.line_lengths[editor.cursor_row];
            break;

        case KEY_PGUP:
            if (editor.cursor_row > EDITOR_VISIBLE_LINES) {
                editor.cursor_row -= EDITOR_VISIBLE_LINES;
            } else {
                editor.cursor_row = 0;
            }
            if (editor.cursor_col > editor.line_lengths[editor.cursor_row]) {
                editor.cursor_col = editor.line_lengths[editor.cursor_row];
            }
            break;

        case KEY_PGDN:
            editor.cursor_row += EDITOR_VISIBLE_LINES;
            if (editor.cursor_row >= editor.line_count) {
                editor.cursor_row = editor.line_count - 1;
            }
            if (editor.cursor_col > editor.line_lengths[editor.cursor_row]) {
                editor.cursor_col = editor.line_lengths[editor.cursor_row];
            }
            break;
    }
}

static void editor_process_key(void) {
    char c = keyboard_getchar();

    // Clear status message on any key
    editor.status_msg[0] = '\0';

    // Handle backspace first (before Ctrl check, since '\b' = 8 is in Ctrl range)
    if (c == '\b') {
        editor_delete_char();
        return;
    }

    // Handle newline/enter
    if (c == '\n') {
        editor_insert_newline();
        return;
    }

    // Handle tab
    if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            editor_insert_char(' ');
        }
        return;
    }

    // Handle Ctrl combinations (1-26 = Ctrl+A through Ctrl+Z)
    // Note: Skip values that are special chars ('\b'=8, '\t'=9, '\n'=10)
    if (c >= 1 && c <= 26) {
        switch (c) {
            case 19:  // Ctrl+S
                editor_save_file();
                break;

            case 24:  // Ctrl+X
                if (editor.modified) {
                    str_cpy(editor.status_msg, "Unsaved changes! Press Ctrl+X again to quit");
                    editor_refresh_screen();
                    c = keyboard_getchar();
                    if (c == 24) {
                        editor.running = FALSE;
                    }
                } else {
                    editor.running = FALSE;
                }
                break;
        }
        return;
    }

    // Handle special keys (arrow keys, etc.)
    if ((uint8_t)c >= KEY_UP && (uint8_t)c <= KEY_F12) {
        editor_move_cursor((uint8_t)c);
        return;
    }

    // Handle printable characters
    if (c >= 32 && c < 127) {
        editor_insert_char(c);
    }
    else if (c == '\t') {
        // Insert spaces for tab
        for (int i = 0; i < 4; i++) {
            editor_insert_char(' ');
        }
    }
}

void editor_run(const char *filename) {
    editor_init(filename);
    editor_load_file();

    keyboard_set_echo(FALSE);
    vga_clear();

    while (editor.running) {
        editor_refresh_screen();
        editor_process_key();
    }

    keyboard_set_echo(TRUE);
    vga_clear();
}
