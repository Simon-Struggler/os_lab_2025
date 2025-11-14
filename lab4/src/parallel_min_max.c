#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для обработки сигналов
volatile sig_atomic_t timeout_occurred = 0;
pid_t *child_pids = NULL;
int pnum_global = 0;

// Обработчик сигнала SIGALRM
void timeout_handler(int sig) {
    timeout_occurred = 1;
    printf("Timeout occurred! Sending SIGKILL to all child processes...\n");
    
    for (int i = 0; i < pnum_global; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0; // 0 означает отсутствие таймаута
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 't'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "ft:", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case 't':
        timeout = atoi(optarg);
        if (timeout <= 0) {
            printf("timeout must be a positive number\n");
            return 1;
        }
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"seconds\"]\n",
           argv[0]);
    return 1;
  }

  // Выделяем память для хранения PID дочерних процессов
  child_pids = malloc(pnum * sizeof(pid_t));
  pnum_global = pnum;

  // Настраиваем обработчик сигнала SIGALRM если задан таймаут
  if (timeout > 0) {
    signal(SIGALRM, timeout_handler);
    alarm(timeout); // Устанавливаем таймер
    printf("Timeout set to %d seconds\n", timeout);
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  // Создаем пайпы или файлы для каждого процесса
  int pipefd[2 * pnum];
  char filenames[pnum][256];
  
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipefd + i * 2) < 0) {
        printf("Pipe failed!\n");
        return 1;
      }
    }
  } else {
    for (int i = 0; i < pnum; i++) {
      snprintf(filenames[i], sizeof(filenames[i]), "min_max_%d.txt", i);
    }
  }

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Вычисляем размер части массива для каждого процесса
  int chunk_size = array_size / pnum;
  int remainder = array_size % pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      child_pids[i] = child_pid; // сохраняем PID дочернего процесса
      
      if (child_pid == 0) {
        // child process
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : start + chunk_size;
        
        // Если есть остаток, добавляем его к последнему процессу
        if (i == pnum - 1 && remainder > 0) {
          end += remainder;
        }

        struct MinMax local_min_max = GetMinMax(array, start, end);

        if (with_files) {
          // use files here
          FILE *file = fopen(filenames[i], "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          // use pipe here
          close(pipefd[i * 2]); // закрываем чтение
          write(pipefd[i * 2 + 1], &local_min_max.min, sizeof(int));
          write(pipefd[i * 2 + 1], &local_min_max.max, sizeof(int));
          close(pipefd[i * 2 + 1]);
        }
        free(array);
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // В родительском процессе закрываем ненужные дескрипторы пайпов
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipefd[i * 2 + 1]); // закрываем запись в родительском процессе
    }
  }

  // Ожидание завершения дочерних процессов с возможностью таймаута
  while (active_child_processes > 0) {
    int status;
    pid_t finished_pid;
    
    // Неблокирующее ожидание с WNOHANG
    finished_pid = waitpid(-1, &status, WNOHANG);
    
    if (finished_pid > 0) {
      // Один из дочерних процессов завершился
      active_child_processes -= 1;
      
      if (WIFEXITED(status)) {
        printf("Child process %d exited normally\n", finished_pid);
      } else if (WIFSIGNALED(status)) {
        printf("Child process %d killed by signal %d\n", finished_pid, WTERMSIG(status));
      }
    } else if (finished_pid == 0) {
      // Дочерние процессы еще работают, проверяем таймаут
      if (timeout_occurred) {
        printf("Timeout! Some processes were terminated\n");
        break;
      }
      // Небольшая пауза чтобы не нагружать CPU
      //usleep(10000); // 10ms
    } else {
      // Ошибка waitpid
      perror("waitpid failed");
      break;
    }
  }

  // Отменяем таймер если он еще не сработал
  if (timeout > 0 && !timeout_occurred) {
    alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  int results_received = 0;
  
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    int result_available = 0;

    if (with_files) {
      // read from files
      FILE *file = fopen(filenames[i], "r");
      if (file != NULL) {
        if (fscanf(file, "%d %d", &min, &max) == 2) {
          result_available = 1;
        }
        fclose(file);
        // Удаляем временный файл
        remove(filenames[i]);
      }
    } else {
      // read from pipes
      // Проверяем, есть ли данные для чтения
      fd_set readfds;
      struct timeval tv;
      
      FD_ZERO(&readfds);
      FD_SET(pipefd[i * 2], &readfds);
      tv.tv_sec = 0;
      tv.tv_usec = 1000; // 1ms timeout
      
      if (select(pipefd[i * 2] + 1, &readfds, NULL, NULL, &tv) > 0) {
        if (FD_ISSET(pipefd[i * 2], &readfds)) {
          read(pipefd[i * 2], &min, sizeof(int));
          read(pipefd[i * 2], &max, sizeof(int));
          result_available = 1;
        }
      }
      close(pipefd[i * 2]);
    }

    if (result_available) {
      results_received++;
      if (min < min_max.min) min_max.min = min;
      if (max > min_max.max) min_max.max = max;
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Results received from %d out of %d processes\n", results_received, pnum);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  if (timeout_occurred) {
    printf("WARNING: Execution terminated due to timeout!\n");
  }
  
  fflush(NULL);
  return 0;
}