#include <stdio.h>

#include "command.h"
#include "errors.h"
#include "parser.h"

int
init_empty_command(Command *c)
{
    return 0;
}

int
init_sequence_command(Command *c, int kind)
{
    return 0;
}

int
append_command_to_sequence(Command *c, Command *cmd)
{
    return 0;
}

int
append_operation_to_sequence(Command *c, int op)
{
    return 0;
}

int
init_pipeline_command(Command *c)
{
    return 0;
}

int
append_to_pipeline(Command *c, Command *cmd)
{
    return 0;
}

int
init_redirect_command(Command *c)
{
    return 0;
}

int
set_rd_command(Command *c, Command *cmd)
{
    return 0;
}

int
init_simple_command(Command *c)
{
   return 0;
}

int
append_word_simple_command(Command *c, char *arg)
{
    return 0;

}

void free_command(Command *c)
{
}

int
main(void)
{
    int r;

    while (1) {

        if ((r = init_parser(stdin)) != 0) {
            fprintf(stderr, "%s\n", error_message(r));
            return 0;
        }

        Command c;
        if ((r = next_command(&c)) == EOF && feof(stdin)) {
            free_parser();
            break;
        } else if (r != 0) {
            fprintf(stderr, "%s\n", error_message(r));
            free_parser();
            continue;
        }

        free_command(&c);
        free_parser();
    }

    return 0;
}
