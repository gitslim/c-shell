#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

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

int
run_command(Command *);

int
simple_cmd(Command *cmd) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);

    } else if (pid == 0) {
        execvp(cmd->argv[0], cmd->argv);
        // exec функции завершаются только при ошибке
        perror("execvp");
        exit(EXIT_FAILURE);

    } else {
        // ожидание завершения конкретного pid
//        do {
//            waitpid(pid, &status, WUNTRACED);
//        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        wait(&status);
    }
    return status;
}

int
redirect_cmd(Command *cmd) {
    int fd;
    int stream;
    switch (cmd->rd_mode) {
        case RD_INPUT:
            fd = open(cmd->rd_path, O_RDONLY);
            stream = STDIN_FILENO;
            break;
        case RD_OUTPUT:
            fd = open(cmd->rd_path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
            stream = STDOUT_FILENO;
            break;
        case RD_APPEND:
            fd = open(cmd->rd_path, O_CREAT | O_WRONLY | O_APPEND, 0666);
            stream = STDOUT_FILENO;
            break;
        default:
            errno = EINVAL;
            perror("redirect_cmd");
            exit(EXIT_FAILURE);
    }

    dup2(fd, stream);
    close(fd);

    int status = run_command(cmd->rd_command);
    return status;
}

int
seq1_cmd(Command *cmd) {
    int status;
    pid_t pid;

    for (int i = 0; i < cmd->seq_size; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);

        } else if (pid == 0) { // child
            int cmd_status = run_command(&cmd->seq_commands[i]);
            exit(cmd_status);

        } else { // parent
            if (i == cmd->seq_size - 1) { // last command in seq
                if (cmd->seq_operations[i] != OP_BACKGROUND) {
                    wait(&status);
                } else {
                    status = EXIT_SUCCESS; // don't wait child
                }
            } else {
                if (cmd->seq_operations[i] != OP_BACKGROUND) {
                    wait(NULL);
                }
            }
        }
    }

    while (wait(NULL) != -1); // wait all childs
    return status;
}

int
seq2_cmd(Command *cmd) {
    int status, skip_cmd;
    int i = 0;

    while (i < cmd->seq_size) {
        status = run_command(&cmd->seq_commands[i]);

        if (i < cmd->seq_size - 1) {
            switch (cmd->seq_operations[i]) {
                case OP_CONJUNCT:
                    skip_cmd = (status == EXIT_SUCCESS) ? 0 : 1;
                    break;
                case OP_DISJUNCT:
                    skip_cmd = (status == EXIT_SUCCESS) ? 1 : 0;
                    break;
                default:
                    errno = EINVAL;
                    perror("seq2_cmd");
                    exit(EXIT_FAILURE);
            }

            if (skip_cmd) {
                i++;
            }
        }
        i++;
    }

    return status;
}

int
pipeline_cmd(Command *cmd) {
    int status;
    pid_t pid;

    int fds[2];
    pipe(fds);

    for (int i = 0; i < cmd->pipeline_size; i++) {
        close(fds[PIPE_WRITE]); // Закрываем дескриптор записи
        dup2(fds[PIPE_READ], STDIN_FILENO);  // Перенаправляем стандартный ввод на канал

        pipe(fds);

        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);

        } else if (pid == 0) { // child
            close(fds[PIPE_READ]); // Закрываем дескриптор чтения
            if (i != cmd->pipeline_size - 1) {
                dup2(fds[PIPE_WRITE],
                     STDOUT_FILENO); // Перенаправляем стандартный вывод на канал если не последняя команда
            }
            int cmd_status = run_command(&cmd->pipeline_commands[i]);
            exit(cmd_status); // обязательно завершаем процесс-потомок

        } else { // parent
            if (i == cmd->pipeline_size - 1) {
                wait(&status); // Ждем завершения последней команды в конвейере
            }
        }
    }

    return status;
}

int
run_command(Command *cmd) {
    switch (cmd->kind) {
        case KIND_SIMPLE:
            return simple_cmd(cmd);
        case KIND_REDIRECT:
            return redirect_cmd(cmd);
        case KIND_PIPELINE:
            return pipeline_cmd(cmd);
        case KIND_SEQ1:
            return seq1_cmd(cmd);
        case KIND_SEQ2:
            return seq2_cmd(cmd);
        default:
            break;
    }
    errno = EINVAL;
    perror("run_command");
    exit(EXIT_FAILURE);
}

/*      TEST        */

int
main(void) {
    // command "uname"

    Command c1_1_1 = {
            .kind = KIND_SIMPLE,
            .argc = 1,
            .argv = (char *[]) {"uname", 0}
    };
    Command c1_1 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c1_1_1,
    };
    run_command(&c1_1);

    // command "echo 1 2 3 > file && wc < file &"

    Command c2_1_1_1 = {
            .kind = KIND_SIMPLE,
            .argc = 4,
            .argv = (char *[]) {"echo", "1", "2", "3", 0}
    };
    Command c2_1_1 = {
            .kind = KIND_REDIRECT,
            .rd_mode = RD_OUTPUT,
            .rd_path = "file",
            .rd_command = &c2_1_1_1
    };
    Command c2_1 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c2_1_1,
    };
    Command c2_2_1_1 = {
            .kind = KIND_SIMPLE,
            .argc = 1,
            .argv = (char *[]) {"wc", 0},
    };
    Command c2_2_1 = {
            .kind = KIND_REDIRECT,
            .rd_mode = RD_INPUT,
            .rd_path = "file",
            .rd_command = &c2_2_1_1,
    };
    Command c2_2 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c2_2_1,
    };
    Command c2 = {
            .kind = KIND_SEQ2,
            .seq_size = 2,
            .seq_commands = (Command[]) {c2_1, c2_2},
            .seq_operations = (int[]) {OP_CONJUNCT},
    };
    Command c2_0 = {
            .kind = KIND_SEQ1,
            .seq_size = 1,
            .seq_commands = &c2,
            .seq_operations = (int[]) {OP_BACKGROUND},
    };
    run_command(&c2_0);

    // command "echo 1 2 3 | wc"

    Command c3 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 2,
            .pipeline_commands = (Command[]) {c2_1_1_1, c2_2_1_1}
    };
    run_command(&c3);

    // command "echo 1 >> file || echo 2 >> file && cat file"
    Command c4_1_1_1 = {
            .kind = KIND_SIMPLE,
            .argc = 2,
            .argv = (char *[]) {"echo", "1", 0},
    };
    Command c4_1_1 = {
            .kind = KIND_REDIRECT,
            .rd_mode = RD_APPEND,
            .rd_path = "file",
            .rd_command = &c4_1_1_1,
    };
    Command c4_1 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c4_1_1,
    };
    Command c4_2_1_1 = {
            .kind = KIND_SIMPLE,
            .argc = 2,
            .argv = (char *[]) {"echo", "2", 0},
    };
    Command c4_2_1 = {
            .kind = KIND_REDIRECT,
            .rd_mode = RD_APPEND,
            .rd_path = "file",
            .rd_command = &c4_2_1_1,
    };
    Command c4_2 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c4_2_1,
    };
    Command c4_3_1 = {
            .kind = KIND_SIMPLE,
            .argc = 2,
            .argv = (char *[]) {"cat", "file", 0},
    };
    Command c4_3 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c4_3_1,
    };
    Command c4 = {
            .kind = KIND_SEQ2,
            .seq_size = 3,
            .seq_commands = (Command[]) {c4_1, c4_2, c4_3},
            .seq_operations = (int[]) {OP_DISJUNCT, OP_CONJUNCT},
    };
    run_command(&c4);

    // command "echo 1 2 3 | wc > file; cat file"
    Command c5_1_1 = {
            .kind = KIND_REDIRECT,
            .rd_mode = RD_OUTPUT,
            .rd_path = "file",
            .rd_command = &c2_2_1_1,
    };
    Command c5_1 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 2,
            .pipeline_commands = (Command[]) {c2_1_1_1, c5_1_1},
    };
    Command c5 = {
            .kind = KIND_SEQ1,
            .seq_size = 2,
            .seq_commands = (Command[]) {c5_1, c4_3},
            .seq_operations = (int[]) {OP_SEQ, OP_SEQ},
    };
    run_command(&c5);

    // command "echo 1 || (echo 2 && echo 3)"
    Command c6_1 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c4_1_1_1,
    };
    Command c6_2_1_1 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c4_2_1_1,
    };
    Command c6_2_1_2_1 = {
            .kind = KIND_SIMPLE,
            .argc = 2,
            .argv = (char *[]) {"echo", "3", 0},
    };
    Command c6_2_1_2 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c6_2_1_2_1,
    };
    Command c6_2_1 = {
            .kind = KIND_SEQ2,
            .seq_size = 2,
            .seq_commands = (Command[]) {c6_2_1_1, c6_2_1_2},
            .seq_operations = (int[]) {OP_CONJUNCT},
    };
    Command c6_2 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 1,
            .pipeline_commands = &c6_2_1,
    };
    Command c6 = {
            .kind = KIND_SEQ2,
            .seq_size = 2,
            .seq_commands = (Command[]) {c6_1, c6_2},
            .seq_operations = (int[]) {OP_DISJUNCT},
    };
    run_command(&c6);

    // command "yes | head"
    Command c7_1 = {
            .kind = KIND_SIMPLE,
            .argc = 1,
            .argv = (char *[]) {"yes", 0},
    };
    Command c7_2 = {
            .kind = KIND_SIMPLE,
            .argc = 1,
            .argv = (char *[]) {"head", 0},
    };
    Command c7 = {
            .kind = KIND_PIPELINE,
            .pipeline_size = 2,
            .pipeline_commands = (Command[]) {c7_1, c7_2},
    };
    run_command(&c7);

    return 0;
}
