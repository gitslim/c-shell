#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "runner.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

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
    int status = EXIT_FAILURE, skip_cmd;
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
    int status = EXIT_FAILURE;
    pid_t pid;

    int fds[2];
    if (pipe(fds) == -1)
        return -1;

    for (int i = 0; i < cmd->pipeline_size; i++) {
        close(fds[PIPE_WRITE]); // Закрываем дескриптор записи
        dup2(fds[PIPE_READ], STDIN_FILENO);  // Перенаправляем стандартный ввод на канал

        if (pipe(fds) == -1)
            return -1;

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
