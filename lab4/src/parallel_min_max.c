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

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0; // 0 means no timeout
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

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
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;
          case 4:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
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
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  if (timeout > 0) {
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
  pid_t child_pids[pnum]; // Сохраняем PID всех дочерних процессов

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
      child_pids[i] = child_pid; // Сохраняем PID дочернего процесса
      
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

  // Ожидание завершения дочерних процессов с таймаутом
  if (timeout > 0) {
    int time_elapsed = 0;
    while (active_child_processes > 0 && time_elapsed < timeout) {
      sleep(1); // Ждем 1 секунду
      time_elapsed++;
      
      // Проверяем, не завершились ли какие-то процессы
      pid_t finished_pid;
      int status;
      while ((finished_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        active_child_processes -= 1;
        if (WIFEXITED(status)) {
          printf("Child process %d exited normally\n", finished_pid);
        } else if (WIFSIGNALED(status)) {
          printf("Child process %d terminated by signal %d\n", finished_pid, WTERMSIG(status));
        }
      }
    }
    
    // Если время вышло, а процессы еще остались - убиваем их
    if (active_child_processes > 0) {
      printf("Timeout reached! Sending SIGKILL to %d remaining child processes\n", active_child_processes);
      for (int i = 0; i < pnum; i++) {
        // Проверяем, жив ли еще процесс
        if (kill(child_pids[i], 0) == 0) {
          // Процесс еще жив - отправляем SIGKILL
          if (kill(child_pids[i], SIGKILL) == 0) {
            printf("Sent SIGKILL to child process %d\n", child_pids[i]);
          } else {
            perror("Failed to send SIGKILL");
          }
        }
      }
      
      // Ждем завершения убитых процессов
      while (active_child_processes > 0) {
        wait(NULL);
        active_child_processes -= 1;
      }
    }
  } else {
    // Без таймаута - обычное ожидание
    while (active_child_processes > 0) {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Собираем результаты только от завершившихся процессов
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    bool result_available = false;

    if (with_files) {
      // read from files
      FILE *file = fopen(filenames[i], "r");
      if (file != NULL) {
        if (fscanf(file, "%d %d", &min, &max) == 2) {
          result_available = true;
        }
        fclose(file);
        // Удаляем временный файл
        remove(filenames[i]);
      }
    } else {
      // read from pipes - используем неблокирующее чтение
      fd_set readfds;
      struct timeval tv;
      
      FD_ZERO(&readfds);
      FD_SET(pipefd[i * 2], &readfds);
      tv.tv_sec = 0;
      tv.tv_usec = 10000; // 10ms timeout
      
      // Проверяем, есть ли данные для чтения
      if (select(pipefd[i * 2] + 1, &readfds, NULL, NULL, &tv) > 0) {
        if (read(pipefd[i * 2], &min, sizeof(int)) == sizeof(int) &&
            read(pipefd[i * 2], &max, sizeof(int)) == sizeof(int)) {
          result_available = true;
        }
      }
      close(pipefd[i * 2]);
    }

    if (result_available) {
      if (min < min_max.min) min_max.min = min;
      if (max > min_max.max) min_max.max = max;
    } else {
      printf("No result from child process %d (may have been terminated)\n", child_pids[i]);
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}