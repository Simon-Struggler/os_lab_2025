#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <inttypes.h>

// Структура для передачи данных между клиентом и сервером
struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

// Функция безопасного умножения по модулю
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

// Функция преобразования строки в uint64_t
int ConvertStringToUI64(const char *str, uint64_t *val);

#endif // COMMON_H