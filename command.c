#include <malloc.h>
#include <string.h>
#include "command.h"

int deep_copy_command(Command *dest, const Command *src) {
    if (dest == NULL || src == NULL) {
        return -1; // Некорректные входные данные
    }

    // Копируем kind
    dest->kind = src->kind;

    // В зависимости от типа команды копируем соответствующие поля
    switch (src->kind) {
        case KIND_SIMPLE:
            // Копируем argc
            dest->argc = src->argc;
            if (src->argc > 0) {
                // Копируем массив argv
//            dest->argv = malloc(src->argc * sizeof(char *));
// +1 под нулевой указатель в конце массива (формат для функций exec..)
                dest->argv = malloc((src->argc + 1) * sizeof(char *));
                if (dest->argv == NULL) {
                    return -1; // Ошибка выделения памяти
                }
                for (int i = 0; i < src->argc; ++i) {
                    dest->argv[i] = strdup(src->argv[i]);
                    if (dest->argv[i] == NULL) {
                        // Ошибка выделения памяти
                        // В этом случае, нужно освободить уже выделенные строки
                        for (int j = 0; j < i; ++j) {
                            free(dest->argv[j]);
                        }
                        free(dest->argv);
                        return -1;
                    }
                }
                dest->argv[src->argc] = 0; // нулевой указатель в конце массива
            } else {
                dest->argv = NULL;
            }
            break;

        case KIND_REDIRECT:
            // Копируем rd_mode
//            dest->rd_mode = src->rd_mode;
            // Копируем rd_path
//            dest->rd_path = strdup(src->rd_path);
//            if (dest->rd_path == NULL) {
//                return -1; // Ошибка выделения памяти
//            }
            if (dest->rd_command != NULL) {
                // Рекурсивно копируем rd_command
                dest->rd_command = malloc(sizeof(Command));
                if (dest->rd_command == NULL) {
                    free(dest->rd_path);
                    return -1; // Ошибка выделения памяти
                }
                if (deep_copy_command(dest->rd_command, src->rd_command) != 0) {
                    free(dest->rd_path);
                    free(dest->rd_command);
                    return -1; // Ошибка копирования
                }
            } else {
                dest->rd_command = NULL;
            }
            break;

        case KIND_PIPELINE:
            // Копируем pipeline_size
            dest->pipeline_size = src->pipeline_size;
            if (src->pipeline_size > 0) {
                // Рекурсивно копируем pipeline_commands
                dest->pipeline_commands = malloc(src->pipeline_size * sizeof(Command));
                if (dest->pipeline_commands == NULL) {
                    return -1; // Ошибка выделения памяти
                }
                for (int i = 0; i < src->pipeline_size; ++i) {
                    if (deep_copy_command(&dest->pipeline_commands[i], &src->pipeline_commands[i]) != 0) {
                        // Ошибка копирования
                        // В этом случае, нужно освободить уже выделенные команды
                        for (int j = 0; j < i; ++j) {
                            free_command(&dest->pipeline_commands[j]);
                        }
                        free(dest->pipeline_commands);
                        return -1;
                    }
                }
            } else {
                dest->pipeline_commands = NULL;
            }
            break;

        case KIND_SEQ1:
        case KIND_SEQ2:
            // Копируем seq_size
            dest->seq_size = src->seq_size;
            if (src->seq_size > 0) {
                // Рекурсивно копируем seq_commands
                dest->seq_commands = malloc(src->seq_size * sizeof(Command));
                if (dest->seq_commands == NULL) {
                    return -1; // Ошибка выделения памяти
                }
                for (int i = 0; i < src->seq_size; ++i) {
                    if (deep_copy_command(&dest->seq_commands[i], &src->seq_commands[i]) != 0) {
                        // Ошибка копирования
                        // В этом случае, нужно освободить уже выделенные команды
                        for (int j = 0; j < i; ++j) {
                            free_command(&dest->seq_commands[j]);
                        }
                        free(dest->seq_commands);
                        return -2;
                    }
                }
            } else {
                dest->seq_commands = NULL;
            }
            if (src->seq_operations != NULL) {
                // Копируем seq_operations
                dest->seq_operations = malloc((src->seq_size - 1) * sizeof(int));
                if (dest->seq_operations == NULL) {
                    // Ошибка выделения памяти
                    free(dest->seq_commands);
                    return -1;
                }
                memcpy(dest->seq_operations, src->seq_operations, (src->seq_size - 1) * sizeof(int));
            } else {
                dest->seq_operations = NULL;
            }
            break;
        default:
            return -1; // неизвестный тип команды
    }

    return 0; // Успешное завершение
}

int copy_command(Command *dest, const Command *src) {
    if (dest == NULL || src == NULL) {
        return -1; // Некорректные входные данные
    }

    // Копируем kind
    dest->kind = src->kind;

    // Копируем аргументы
    dest->argc = src->argc;
//    dest->argv = malloc(src->argc * sizeof(char *));
    dest->argv = malloc((src->argc + 1) * sizeof(char *));
    if (dest->argv == NULL) {
        return -2; // Ошибка выделения памяти
    }

    for (int i = 0; i < src->argc; ++i) {
        dest->argv[i] = strdup(src->argv[i]);
        if (dest->argv[i] == NULL) {
            // Ошибка выделения памяти
            // В этом случае, нужно освободить уже выделенные строки
            for (int j = 0; j < i; ++j) {
                free(dest->argv[j]);
            }
            free(dest->argv);
            return -2;
        }
    }
    dest->argv[src->argc] = 0; // нулевой указатель в конце массива

    return 0; // Успешное завершение
}

int
init_empty_command(Command *c) {
    return 0;
}

int
init_sequence_command(Command *c, int kind) {
    Command cmd = {
            .kind = kind,
            .seq_size = 0,
            .seq_commands = NULL, //(Command[]) {},
            .seq_operations = NULL // (int[]) {}
    };
    *c = cmd;
    return 0;
}

int
append_command_to_sequence(Command *c, Command *cmd) {
    if (c == NULL || cmd == NULL) {
        return -1; // Некорректные входные данные
    }

    // Создаем временную структуру для модификации
    Command c_;
    if (deep_copy_command(&c_, c) != 0) {
        return -1; // Ошибка копирования
    }

    // Увеличиваем размер массива
    c_.seq_commands = realloc(c_.seq_commands, (c_.seq_size + 1) * sizeof(Command *));
    if (c_.seq_commands == NULL) {
        free_command(&c_);
        return -1; // Ошибка выделения памяти
    }

    // Создаем временную структуру для модификации
    Command cmd_;
    if (deep_copy_command(&cmd_, cmd) != 0) {
        return -1; // Ошибка копирования
    }

    c_.seq_commands[c_.seq_size] = cmd_;
    c_.seq_size++;

    *c = c_;
    return 0;
}

int
append_operation_to_sequence(Command *c, int op) {
    if (c == NULL) {
        return -1;
    }

    Command c_;
    if (deep_copy_command(&c_, c) != 0) {
        return -1;
    }

    c_.seq_operations = realloc(c_.seq_operations, c_.seq_size * sizeof(int));
    if (c_.seq_operations == NULL) {
        free_command(&c_);
        return -1;
    }

    c_.seq_operations[c_.seq_size - 1] = op;

    *c = c_;
    return 0;
}

int
init_pipeline_command(Command *c) {
    Command cmd = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 0,
            .pipeline_commands = NULL // (Command[]) {}
    };
    *c = cmd;
    return 0;
}

int
append_to_pipeline(Command *c, Command *cmd) {
    if (c == NULL || cmd == NULL) {
        return -1;
    }

    Command c_;
    if (deep_copy_command(&c_, c) != 0) {
        return -1;
    }

    c_.pipeline_commands = realloc(c_.pipeline_commands, (c_.pipeline_size + 1) * sizeof(Command *));
    if (c_.pipeline_commands == NULL) {
        free_command(&c_);
        return -1;
    }

    Command cmd_;
    if (deep_copy_command(&cmd_, cmd) != 0) {
        return -1;
    }

    c_.pipeline_commands[c_.pipeline_size] = cmd_;
    c_.pipeline_size++;

    *c = c_;
    return 0;
}

int
init_redirect_command(Command *c) {
    Command cmd = {
            .kind = KIND_REDIRECT,
//            .rd_mode = -1,
//            .rd_path = (char *) calloc(1, sizeof(char *)), // пустая строка
            .rd_command = NULL
    };
    *c = cmd;
    return 0;
}

int
set_rd_command(Command *c, Command *cmd) {
    if (c == NULL || cmd == NULL) {
        return -1;
    }

    Command c_;
    if (deep_copy_command(&c_, c) != 0) {
        return -1;
    }

    Command cmd_;
    if (deep_copy_command(&cmd_, cmd) != 0) {
        return -1;
    }
    c_.rd_command = &cmd_;

    *c = c_;
    return 0;
}

int
init_simple_command(Command *c) {
    Command cmd = {
            .kind = KIND_SIMPLE,
            .argc = 0,
            .argv = NULL //calloc(1, sizeof(char *)) //
    };
    *c = cmd;
    return 0;
}

int
append_word_simple_command(Command *c, char *arg) {
    if (c == NULL || arg == NULL) {
        return -1; // Некорректные входные данные
    }

    // Создаем временную структуру для модификации
    Command c_;
    if (deep_copy_command(&c_, c) != 0) {
        return -2; // Ошибка копирования
    }

    // Увеличиваем размер массива на 1 и выделяем память под новое слово
//    c_.argv = realloc(c_.argv, (c_.argc + 1) * sizeof(char *));
    c_.argv = realloc(c_.argv, (c_.argc + 2) * sizeof(char *)); // 2 - потому что добавляем в конец нулевой указатель
    if (c_.argv == NULL) {
        free_command(&c_);
        return -2; // Ошибка выделения памяти
    }

    // Копируем новое слово в конец массива
    c_.argv[c_.argc] = strdup(arg);
    if (c_.argv[c_.argc] == NULL) {
        free_command(&c_);
        return -2; // Ошибка выделения памяти
    }
    c_.argv[c_.argc + 1] = 0; // добавляем нулeвой указатель в концец массива

    // Увеличиваем счетчик аргументов
    c_.argc++;

    // Присваиваем результат обратно в оригинальную структуру
    *c = c_;

    return 0; // Успешное завершение
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
