#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>

#include "sum.h"
#include "utils.h"

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  long long *result = malloc(sizeof(long long));
  *result = Sum(sum_args);
  return (void *)result;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  // Парсинг аргументов командной строки
  static struct option options[] = {
      {"threads_num", required_argument, 0, 0},
      {"array_size", required_argument, 0, 0},
      {"seed", required_argument, 0, 0},
      {0, 0, 0, 0}
  };

  int option_index = 0;
  while (true) {
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            threads_num = atoi(optarg);
            if (threads_num <= 0) {
              printf("threads_num must be positive\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;
          case 2:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be positive\n");
              return 1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (threads_num == 0 || array_size == 0 || seed == 0) {
    printf("Usage: %s --threads_num \"num\" --array_size \"num\" --seed \"num\"\n", 
           argv[0]);
    return 1;
  }

  // Генерация массива
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];

  // Распределение работы между потоками
  int chunk_size = array_size / threads_num;
  int remainder = array_size % threads_num;

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * chunk_size;
    args[i].end = (i == threads_num - 1) ? array_size : args[i].begin + chunk_size;
    
    // Если есть остаток, добавляем его к последнему потоку
    if (i == threads_num - 1 && remainder > 0) {
      args[i].end += remainder;
    }
  }

  // Замер времени начала вычислений
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создание потоков
  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }

  // Сбор результатов
  long long total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    long long *partial_sum;
    pthread_join(threads[i], (void **)&partial_sum);
    total_sum += *partial_sum;
    free(partial_sum);
  }

  // Замер времени окончания вычислений
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  // Вычисление времени выполнения
  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  printf("Total: %lld\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  return 0;
}