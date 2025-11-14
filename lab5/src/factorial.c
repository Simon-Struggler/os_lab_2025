#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>

long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int start;
    int end;
    int mod;
} thread_data_t;

// Вычисление произведения в диапазоне по модулю
void* partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long long partial_result = 1;
    
    for (int i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * i) % data->mod;
    }
    
    // Захватываем мьютекс для обновления общего результата
    pthread_mutex_lock(&mutex);
    result = (result * partial_result) % data->mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}


void parse_arguments(int argc, char* argv[], int* k, int* pnum, int* mod) {
    *k = 10;
    *pnum = 4;
    *mod = 1000000007;
    
    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                *k = atoi(optarg);
                break;
            case 'p':
                *pnum = atoi(optarg);
                break;
            case 'm':
                *mod = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Использование: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", argv[0]);
                exit(1);
        }
    }
    
    if (*k < 0) {
        fprintf(stderr, "Ошибка: k не может быть отрицательным\n");
        exit(1);
    }
    if (*pnum <= 0) {
        fprintf(stderr, "Ошибка: количество потоков должно быть положительным\n");
        exit(1);
    }
    if (*mod <= 0) {
        fprintf(stderr, "Ошибка: модуль должен быть положительным\n");
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    int k, pnum, mod;

    parse_arguments(argc, argv, &k, &pnum, &mod);
    
    printf("Вычисление %d! mod %d с использованием %d потоков\n", k, mod, pnum);
    
    // Особые случаи
    if (k == 0 || k == 1) {
        printf("Результат: 1\n");
        return 0;
    }
    
    if (pnum > k) {
        pnum = k;
        printf("Количество потоков уменьшено до %d\n", pnum);
    }
    
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        int numbers_for_this_thread = numbers_per_thread;
        if (i < remainder) {
            numbers_for_this_thread++;
        }
        
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_for_this_thread - 1;
        thread_data[i].mod = mod;
        
        printf("Поток %d: числа от %d до %d\n", i, thread_data[i].start, thread_data[i].end);
        
        current_start += numbers_for_this_thread;
        
        if (pthread_create(&threads[i], NULL, partial_factorial, &thread_data[i]) != 0) {
            perror("Ошибка при создании потока");
            exit(1);
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка при ожидании потока");
            exit(1);
        }
    }
    
    // Уничтожение мьютекса
    pthread_mutex_destroy(&mutex);
    
    printf("Результат: %lld\n", result);
    
    return 0;
}