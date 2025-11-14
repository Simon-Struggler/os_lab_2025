#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

int avoid_deadlock = 0;
volatile int thread1_completed = 0;
volatile int thread2_completed = 0;
volatile int deadlock_detected = 0;

void* thread1_function(void* arg) {
    if (avoid_deadlock) {
        printf("Поток 1: пытается захватить мьютекс 1\n");
        pthread_mutex_lock(&mutex1);
        printf("Поток 1: захватил мьютекс 1\n");
        
        sleep(1);
        
        printf("Поток 1: пытается захватить мьютекс 2\n");
        pthread_mutex_lock(&mutex2);
        printf("Поток 1: захватил мьютекс 2\n");
    } else {
        printf("Поток 1: пытается захватить мьютекс 1\n");
        pthread_mutex_lock(&mutex1);
        printf("Поток 1: захватил мьютекс 1\n");
        
        sleep(2);
        
        printf("Поток 1: пытается захватить мьютекс 2\n");
        pthread_mutex_lock(&mutex2);
        printf("Поток 1: захватил мьютекс 2\n");
    }
    
    printf("Поток 1: выполняет работу в критической секции\n");
    sleep(1);
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    printf("Поток 1: завершил работу\n");
    thread1_completed = 1;
    
    return NULL;
}

void* thread2_function(void* arg) {
    if (avoid_deadlock) {
        printf("Поток 2: пытается захватить мьютекс 1\n");
        pthread_mutex_lock(&mutex1);
        printf("Поток 2: захватил мьютекс 1\n");
        
        sleep(1);
        
        printf("Поток 2: пытается захватить мьютекс 2\n");
        pthread_mutex_lock(&mutex2);
        printf("Поток 2: захватил мьютекс 2\n");
    } else {
        printf("Поток 2: пытается захватить мьютекс 2\n");
        pthread_mutex_lock(&mutex2);
        printf("Поток 2: захватил мьютекс 2\n");
        
        sleep(1);
        
        printf("Поток 2: пытается захватить мьютекс 1\n");
        pthread_mutex_lock(&mutex1);
        printf("Поток 2: захватил мьютекс 1\n");
    }
    
    printf("Поток 2: выполняет работу в критической секции\n");
    sleep(1);
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    printf("Поток 2: завершил работу\n");
    thread2_completed = 1;
    
    return NULL;
}

// Функция для мониторинга состояния потоков
void* monitor_thread(void* arg) {
    sleep(5); // Ждем 5 секунд
    
    if (!thread1_completed || !thread2_completed) {
        deadlock_detected = 1;
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t thread1, thread2, monitor;
    
    if (argc > 1 && strcmp(argv[1], "--avoid") == 0) {
        avoid_deadlock = 1;
        printf("=== Режим избежания Deadlock ===\n");
        printf("Мьютексы всегда захватываются в порядке: 1 -> 2\n\n");
    } else {
        printf("=== Режим Deadlock ===\n");
        printf("Поток 1: мьютекс 1 -> 2 | Поток 2: мьютекс 2 -> 1\n\n");
    }
    
    // Создание потоков
    if (pthread_create(&thread1, NULL, thread1_function, NULL) != 0) {
        perror("Ошибка создания потока 1");
        exit(1);
    }
    
    if (pthread_create(&thread2, NULL, thread2_function, NULL) != 0) {
        perror("Ошибка создания потока 2");
        exit(1);
    }
    
    // Создаем поток-монитор для обнаружения deadlock
    if (pthread_create(&monitor, NULL, monitor_thread, NULL) != 0) {
        perror("Ошибка создания монитора");
        exit(1);
    }
    
    // Ожидаем завершения основного потока-монитора
    pthread_join(monitor, NULL);
    
    printf("\n=== Результаты ===\n");
    
    if (thread1_completed && thread2_completed) {
        printf("ОБА потока успешно завершились!\n");
        printf("Deadlock не произошел\n");
    } else {
        printf("Состояние потоков:\n");
        printf("Поток 1: %s\n", thread1_completed ? "завершился" : "ЗАБЛОКИРОВАН");
        printf("Поток 2: %s\n", thread2_completed ? "завершился" : "ЗАБЛОКИРОВАН");
        
        if (deadlock_detected && !avoid_deadlock) {
            printf("\n=== ОБНАРУЖЕН DEADLOCK! ===\n");
            printf("Потоки взаимно заблокированы:\n");
            printf("- Поток 1 ждет мьютекс 2, который удерживает поток 2\n");
            printf("- Поток 2 ждет мьютекс 1, который удерживает поток 1\n");
        }
    }
    
    // Уничтожение мьютексов
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    
    return 0;
}