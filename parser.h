#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "command.h"

// Инициализация парсера с указанием файла ввода
int
init_parser(FILE *input);

// Освобождение ресурсов, связанных с парсером
void
free_parser(void);

// Получение следующей команды из входного потока
int
next_command(Command *c);

#endif
