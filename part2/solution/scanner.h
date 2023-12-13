#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>

// Enum для представления видов токенов
enum TokenKind {
    T_EOF,
    T_NEWLINE,
    T_OPEN,
    T_CLOSE,
    T_SEQ,
    T_BACK,
    T_CONJ,
    T_DISJ,
    T_PIPE,
    T_IN,
    T_OUT,
    T_APPEND,
    T_WORD
};

// Структура для представления токена
typedef struct {
    enum TokenKind kind;  // Вид токена
    char *text;           // Текст токена (если применимо)
    size_t len;           // Длина текста
    size_t capacity;      // Вместимость текста
} Token;

// Инициализация сканера с указанием файла ввода
int init_scanner(FILE *f);

// Освобождение ресурсов, связанных со сканером
void free_scanner(void);

// Получение следующего токена из входного потока
int next_token(Token *token);

// Освобождение ресурсов, связанных с токеном
void free_token(Token *token);

#endif
