#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "fs.h"

static struct shell_state shell;

// External command table (defined in commands.c)
extern const struct command commands[];

void shell_init(void) {
    shell.input_pos = 0;
    shell.argc = 0;
    shell.running = FALSE;
    mem_set(shell.input_buf, 0, MAX_CMD_LEN);
}

void shell_print_prompt(void) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("MonkeyOS");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_putchar(':');
    vga_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
    vga_puts(fs_get_cwd());
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("$ ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

void shell_read_line(void) {
    shell.input_pos = 0;
    mem_set(shell.input_buf, 0, MAX_CMD_LEN);

    keyboard_set_echo(FALSE);

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putchar('\n');
            shell.input_buf[shell.input_pos] = '\0';
            break;
        }
        else if (c == '\b') {
            if (shell.input_pos > 0) {
                shell.input_pos--;
                shell.input_buf[shell.input_pos] = '\0';
                vga_putchar('\b');
            }
        }
        else if (c >= 32 && c < 127) {  // Printable ASCII
            if (shell.input_pos < MAX_CMD_LEN - 1) {
                shell.input_buf[shell.input_pos++] = c;
                vga_putchar(c);
            }
        }
        // Ignore special keys in command line
    }

    keyboard_set_echo(TRUE);
}

void shell_parse_command(void) {
    shell.argc = 0;
    char *p = shell.input_buf;

    while (*p && shell.argc < MAX_ARGS) {
        // Skip whitespace
        while (*p == ' ') p++;
        if (*p == '\0') break;

        // Copy argument
        uint8_t len = 0;
        while (*p && *p != ' ' && len < MAX_ARG_LEN - 1) {
            shell.args[shell.argc][len++] = *p++;
        }
        shell.args[shell.argc][len] = '\0';
        shell.argc++;
    }
}

void shell_execute(void) {
    if (shell.argc == 0) return;  // Empty command

    // Find matching command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (str_cmp(shell.args[0], commands[i].name) == 0) {
            commands[i].handler(shell.argc, shell.args);
            return;
        }
    }

    // Command not found
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_puts("Unknown command: ");
    vga_puts(shell.args[0]);
    vga_putchar('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("Type 'help' for available commands.\n");
}

const char* shell_get_arg(int index) {
    if (index < 0 || index >= shell.argc) {
        return "";
    }
    return shell.args[index];
}

void shell_run(void) {
    shell_init();
    shell.running = TRUE;

    vga_clear();
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("MonkeyOS Shell v1.0\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("Type 'help' for available commands.\n\n");

    while (shell.running) {
        shell_print_prompt();
        shell_read_line();
        shell_parse_command();
        shell_execute();
    }
}
