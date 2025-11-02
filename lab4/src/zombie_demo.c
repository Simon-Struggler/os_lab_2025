#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_zombies>\n", argv[0]);
        return 1;
    }

    int num_zombies = atoi(argv[1]);
    if (num_zombies <= 0) {
        printf("Number of zombies must be positive\n");
        return 1;
    }

    printf("Parent process PID: %d\n", getpid());
    printf("Creating %d zombie processes...\n", num_zombies);
    printf("Run 'ps aux | grep Z' in another terminal to see zombies\n");
    printf("Press Enter to clean up zombies and exit...\n");

    pid_t child_pids[num_zombies];

    // Создаем дочерние процессы, которые сразу завершаются
    for (int i = 0; i < num_zombies; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            return 1;
        } else if (pid == 0) {
            // Дочерний процесс
            printf("Child process %d (PID: %d) created and exiting immediately\n", 
                   i, getpid());
            exit(0); // Немедленно завершаемся
        } else {
            // Родительский процесс - сохраняем PID
            child_pids[i] = pid;
        }
    }

    // Родительский процесс НЕ вызывает wait() - создаем зомби
    printf("\n%d zombie processes created!\n", num_zombies);
    printf("Zombie PIDs: ");
    for (int i = 0; i < num_zombies; i++) {
        printf("%d ", child_pids[i]);
    }
    printf("\n");

    // Ждем нажатия Enter
    getchar();

    // Теперь очищаем зомби-процессы
    printf("Cleaning up zombies...\n");
    for (int i = 0; i < num_zombies; i++) {
        int status;
        pid_t result = waitpid(child_pids[i], &status, 0);
        if (result > 0) {
            printf("Reaped zombie process %d\n", child_pids[i]);
        }
    }

    printf("All zombies cleaned up. Exiting...\n");
    return 0;
}