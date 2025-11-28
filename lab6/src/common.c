#include "common.h"
#include <errno.h>
#include <stdlib.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}

int ConvertStringToUI64(const char *str, uint64_t *val) {
    if (str == NULL || val == NULL) {
        return 0;
    }
    
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    
    if (errno == ERANGE) {
        return 0;
    }

    if (errno != 0 || *end != '\0') {
        return 0;
    }

    *val = i;
    return 1;
}