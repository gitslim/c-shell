#ifndef COMMAND_H
#define COMMAND_H


//command types
enum {
    KIND_SIMPLE,
    KIND_REDIRECT,
    KIND_PIPELINE,
    KIND_SEQ1,
    KIND_SEQ2
};

//sequence types
enum {
    //command: &&
    OP_CONJUNCT,

    //command: &
    OP_BACKGROUND,

    //command: ||
    OP_DISJUNCT,

    //command: ;
    OP_SEQ
};

//types of redirects
enum {
    //command: <
    RD_INPUT,

    //command: >
    RD_OUTPUT,

    //command: >>
    RD_APPEND
};


//command tree
typedef struct Command {
    int kind;

    union {
        //simple
        struct {
            int argc;
            char **argv;
        };

        //redirect
        struct {
            int rd_mode;
            char *rd_path;
            struct Command *rd_command;
        };

        //pipeline
        struct {
            int pipeline_size;
            struct Command *pipeline_commands;
        };

        //sequence
        struct {
            int seq_size;
            struct Command *seq_commands;
            int *seq_operations;
        };
    };
} Command;

// Инициализация пустой команды
int init_empty_command(Command *c);

// Инициализация последовательности команд
int init_sequence_command(Command *c, int kind);

// Добавление команды к последовательности
int append_command_to_sequence(Command *c, Command *cmd);

// Добавление операции к последовательности
int append_operation_to_sequence(Command *c, int op);

// Инициализация команды для конвейера
int init_pipeline_command(Command *c);

// Добавление команды к конвейеру
int append_to_pipeline(Command *c, Command *cmd);

// Инициализация команды для перенаправления
int init_redirect_command(Command *c);

// Установка команды для перенаправления
int set_rd_command(Command *c, Command *cmd);

// Инициализация простой команды
int init_simple_command(Command *c);

// Добавление аргумента к простой команде
int append_word_simple_command(Command *c, char *arg);

// Освобождение ресурсов, связанных с командой
void free_command(Command *c);

#endif
