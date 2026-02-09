#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "fs.h"
#include "editor.h"

// Forward declarations
static void cmd_help(int argc, char args[][MAX_ARG_LEN]);
static void cmd_clear(int argc, char args[][MAX_ARG_LEN]);
static void cmd_pwd(int argc, char args[][MAX_ARG_LEN]);
static void cmd_ls(int argc, char args[][MAX_ARG_LEN]);
static void cmd_cd(int argc, char args[][MAX_ARG_LEN]);
static void cmd_mkdir(int argc, char args[][MAX_ARG_LEN]);
static void cmd_touch(int argc, char args[][MAX_ARG_LEN]);
static void cmd_change(int argc, char args[][MAX_ARG_LEN]);
static void cmd_format(int argc, char args[][MAX_ARG_LEN]);

// Command table
const struct command commands[] = {
    {"help",   cmd_help,   "Show available commands"},
    {"clear",  cmd_clear,  "Clear the screen"},
    {"pwd",    cmd_pwd,    "Print working directory"},
    {"ls",     cmd_ls,     "List directory contents"},
    {"cd",     cmd_cd,     "Change directory"},
    {"mkdir",  cmd_mkdir,  "Create a directory"},
    {"touch",  cmd_touch,  "Create an empty file"},
    {"change", cmd_change, "Edit a file (nano-like)"},
    {"format", cmd_format, "Format the filesystem"},
    {NULL, NULL, NULL}
};

static void cmd_help(int argc, char args[][MAX_ARG_LEN]) {
    (void)argc;
    (void)args;

    vga_puts("\nAvailable commands:\n");
    vga_puts("------------------\n");

    for (int i = 0; commands[i].name != NULL; i++) {
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        vga_puts("  ");
        vga_puts(commands[i].name);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

        // Padding
        int len = str_len(commands[i].name);
        for (int j = len; j < 10; j++) {
            vga_putchar(' ');
        }

        vga_puts("- ");
        vga_puts(commands[i].help);
        vga_putchar('\n');
    }
    vga_putchar('\n');
}

static void cmd_clear(int argc, char args[][MAX_ARG_LEN]) {
    (void)argc;
    (void)args;
    vga_clear();
}

static void cmd_pwd(int argc, char args[][MAX_ARG_LEN]) {
    (void)argc;
    (void)args;

    if (!fs_is_mounted()) {
        vga_puts("Filesystem not mounted.\n");
        return;
    }

    vga_puts(fs_get_cwd());
    vga_putchar('\n');
}

static void cmd_ls(int argc, char args[][MAX_ARG_LEN]) {
    (void)argc;
    (void)args;

    if (!fs_is_mounted()) {
        vga_puts("Filesystem not mounted. Use 'format' to create one.\n");
        return;
    }

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("Directory: ");
    vga_puts(fs_get_cwd());
    vga_putchar('\n');
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    fs_list_dir();
}

static void cmd_cd(int argc, char args[][MAX_ARG_LEN]) {
    if (!fs_is_mounted()) {
        vga_puts("Filesystem not mounted.\n");
        return;
    }

    if (argc < 2) {
        // cd with no args goes to root
        fs_chdir("/");
        return;
    }

    if (!fs_chdir(args[1])) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("cd: ");
        vga_puts(args[1]);
        vga_puts(": No such directory\n");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

static void cmd_mkdir(int argc, char args[][MAX_ARG_LEN]) {
    if (!fs_is_mounted()) {
        vga_puts("Filesystem not mounted.\n");
        return;
    }

    if (argc < 2) {
        vga_puts("Usage: mkdir <dirname>\n");
        return;
    }

    if (fs_mkdir(args[1])) {
        vga_puts("Directory created: ");
        vga_puts(args[1]);
        vga_putchar('\n');
    } else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("mkdir: Cannot create directory '");
        vga_puts(args[1]);
        vga_puts("'\n");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

static void cmd_touch(int argc, char args[][MAX_ARG_LEN]) {
    if (!fs_is_mounted()) {
        vga_puts("Filesystem not mounted.\n");
        return;
    }

    if (argc < 2) {
        vga_puts("Usage: touch <filename>\n");
        return;
    }

    if (fs_exists(args[1])) {
        vga_puts("File already exists: ");
        vga_puts(args[1]);
        vga_putchar('\n');
        return;
    }

    if (fs_create(args[1])) {
        vga_puts("File created: ");
        vga_puts(args[1]);
        vga_putchar('\n');
    } else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("touch: Cannot create file '");
        vga_puts(args[1]);
        vga_puts("'\n");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

static void cmd_change(int argc, char args[][MAX_ARG_LEN]) {
    if (!fs_is_mounted()) {
        vga_puts("Filesystem not mounted.\n");
        return;
    }

    if (argc < 2) {
        vga_puts("Usage: change <filename>\n");
        return;
    }

    editor_run(args[1]);
}

static void cmd_format(int argc, char args[][MAX_ARG_LEN]) {
    (void)argc;
    (void)args;

    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("WARNING: This will erase all data on the disk!\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("Type 'yes' to confirm: ");

    // Read confirmation
    char confirm[16];
    int pos = 0;
    keyboard_set_echo(FALSE);

    while (1) {
        char c = keyboard_getchar();
        if (c == '\n') {
            confirm[pos] = '\0';
            vga_putchar('\n');
            break;
        }
        else if (c == '\b' && pos > 0) {
            pos--;
            vga_putchar('\b');
        }
        else if (c >= 32 && c < 127 && pos < 15) {
            confirm[pos++] = c;
            vga_putchar(c);
        }
    }
    keyboard_set_echo(TRUE);

    if (str_cmp(confirm, "yes") == 0) {
        if (fs_format()) {
            vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            vga_puts("Filesystem formatted successfully.\n");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        } else {
            vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
            vga_puts("Failed to format filesystem.\n");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }
    } else {
        vga_puts("Format cancelled.\n");
    }
}
