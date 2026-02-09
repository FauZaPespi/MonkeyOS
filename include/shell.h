#ifndef SHELL_H
#define SHELL_H

#include "types.h"

#define MAX_CMD_LEN     128
#define MAX_ARGS        8
#define MAX_ARG_LEN     64

// Shell state
struct shell_state {
    char     input_buf[MAX_CMD_LEN];
    uint8_t  input_pos;

    char     args[MAX_ARGS][MAX_ARG_LEN];
    uint8_t  argc;

    bool     running;
};

// Shell functions
void shell_init(void);
void shell_run(void);
void shell_print_prompt(void);
void shell_read_line(void);
void shell_parse_command(void);
void shell_execute(void);

// Command handler type
typedef void (*cmd_handler_t)(int argc, char args[][MAX_ARG_LEN]);

// Command structure
struct command {
    const char *name;
    cmd_handler_t handler;
    const char *help;
};

// Get argument by index (returns empty string if not exists)
const char* shell_get_arg(int index);

#endif
