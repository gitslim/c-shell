#ifndef RUNNER_H
#define RUNNER_H

#endif

#include "command.h"

int
run_command(Command *c);

int
simple_cmd(Command *cmd);

int
redirect_cmd(Command *cmd);

int
seq1_cmd(Command *cmd);

int
seq2_cmd(Command *cmd);

int
pipeline_cmd(Command *cmd);