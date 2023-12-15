#include <malloc.h>
#include <string.h>
#include "command.h"

static Command _cmd;

int
init_empty_command(Command *c) {
    return 0;
}

int
init_sequence_command(Command *c, int kind) {
    _cmd.kind = kind;
    _cmd.seq_size = 0;
    _cmd.seq_commands = NULL;
    _cmd.seq_operations = NULL;
    *c = _cmd;
    return 0;
}

int
append_command_to_sequence(Command *c, Command *cmd) {
    _cmd = *c;
    _cmd.seq_size++;
    _cmd.seq_commands = realloc(_cmd.seq_commands, _cmd.seq_size * sizeof(Command));
    _cmd.seq_commands[_cmd.seq_size - 1] = *cmd;
    *c = _cmd;
    return 0;
}

int
append_operation_to_sequence(Command *c, int op) {
    _cmd = *c;
    _cmd.seq_operations = realloc(_cmd.seq_operations, (_cmd.seq_size) * sizeof(int));
    _cmd.seq_operations[_cmd.seq_size - 1] = op;
    *c = _cmd;
    return 0;
}

int
init_pipeline_command(Command *c) {
    _cmd.kind = KIND_PIPELINE;
    _cmd.pipeline_size = 0;
    _cmd.pipeline_commands = NULL;
    *c = _cmd;
    return 0;
}

int
append_to_pipeline(Command *c, Command *cmd) {
    _cmd = *c;
    _cmd.pipeline_size++;
    _cmd.pipeline_commands = realloc(_cmd.pipeline_commands, _cmd.pipeline_size * sizeof(Command));
    _cmd.pipeline_commands[_cmd.pipeline_size - 1] = *cmd;
    *c = _cmd;
    return 0;
}

int
init_redirect_command(Command *c) {
    _cmd.kind = KIND_REDIRECT;
    _cmd.rd_command = NULL;
    *c = _cmd;
    return 0;
}

int
set_rd_command(Command *c, Command *cmd) {
    _cmd = *c;
    _cmd.rd_command = cmd;
    *c = _cmd;
    return 0;
}

int
init_simple_command(Command *c) {
    _cmd.kind = KIND_SIMPLE;
    _cmd.argc = 0;
    _cmd.argv = NULL;
    *c = _cmd;
    return 0;
}

int
append_word_simple_command(Command *c, char *arg) {
    _cmd = *c;
    _cmd.argc++;
    _cmd.argv = realloc(_cmd.argv, (_cmd.argc + 1) * sizeof(char *));
    _cmd.argv[_cmd.argc - 1] = strdup(arg);
    _cmd.argv[_cmd.argc] = 0; // нулевой указатель в конце массива
    *c = _cmd;
    return 0;
}

void
free_command(Command *c) {
    if (c == NULL) {
        return;
    }

    // В зависимости от типа команды освобождаем соответствующие ресурсы
    switch (c->kind) {
        case KIND_SIMPLE:
            for (int i = 0; i < c->argc; i++) {
                free(c->argv[i]);
            }
            free(c->argv);
            break;

        case KIND_REDIRECT:
            free(c->rd_path);
            free_command(c->rd_command);
            free(c->rd_command);
            break;

        case KIND_PIPELINE:
            for (int i = 0; i < c->pipeline_size; i++) {
                free_command(&c->pipeline_commands[i]);
            }
            free(c->pipeline_commands);
            break;

        case KIND_SEQ1:
        case KIND_SEQ2:
            for (int i = 0; i < c->seq_size; i++) {
                free_command(&c->seq_commands[i]);
            }
            free(c->seq_commands);
            free(c->seq_operations);
            break;
        default:
            break; // неизвестный тип команды
    }
}
