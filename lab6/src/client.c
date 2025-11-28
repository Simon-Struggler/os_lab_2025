#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"

struct Server {
    char ip[255];
    int port;
};

struct ClientThreadArgs {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};

void* ClientThread(void* args) {
    struct ClientThreadArgs* thread_args = (struct ClientThreadArgs*)args;
    
    struct hostent *hostname = gethostbyname(thread_args->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
        pthread_exit(NULL);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(thread_args->server.port);
    
    if (hostname->h_addr_list[0] != NULL) {
        memcpy(&server.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
    } else {
        fprintf(stderr, "No address found for %s\n", thread_args->server.ip);
        pthread_exit(NULL);
    }

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        pthread_exit(NULL);
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection failed to %s:%d\n", 
                thread_args->server.ip, thread_args->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &thread_args->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed\n");
        close(sck);
        pthread_exit(NULL);
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Recieve failed\n");
        close(sck);
        pthread_exit(NULL);
    }

    memcpy(&thread_args->result, response, sizeof(uint64_t));
    printf("Received from server %s:%d: %"PRIu64"\n", 
           thread_args->server.ip, thread_args->server.port, thread_args->result);

    close(sck);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};

    while (true) {
        static struct option options[] = {{"k", required_argument, 0, 0},
                                        {"mod", required_argument, 0, 0},
                                        {"servers", required_argument, 0, 0},
                                        {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                if (!ConvertStringToUI64(optarg, &k)) {
                    fprintf(stderr, "Invalid k value: %s\n", optarg);
                    return 1;
                }
                break;
            case 1:
                if (!ConvertStringToUI64(optarg, &mod)) {
                    fprintf(stderr, "Invalid mod value: %s\n", optarg);
                    return 1;
                }
                break;
            case 2:
                memcpy(servers_file, optarg, strlen(optarg));
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == 0 || mod == 0 || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
                argv[0]);
        return 1;
    }

    FILE* file = fopen(servers_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
        return 1;
    }

    struct Server* servers = NULL;
    int servers_num = 0;
    char line[255];
    
    while (fgets(line, sizeof(line), file)) {
        char ip[255];
        int port;
        
        if (sscanf(line, "%254[^:]:%d", ip, &port) == 2) {
            servers_num++;
            servers = realloc(servers, sizeof(struct Server) * servers_num);
            strcpy(servers[servers_num - 1].ip, ip);
            servers[servers_num - 1].port = port;
        }
    }
    fclose(file);

    if (servers_num == 0) {
        fprintf(stderr, "No valid servers found in file\n");
        free(servers);
        return 1;
    }

    printf("Found %d servers\n", servers_num);

    pthread_t threads[servers_num];
    struct ClientThreadArgs thread_args[servers_num];
    
    uint64_t numbers_per_server = k / servers_num;
    uint64_t remainder = k % servers_num;
    uint64_t current_start = 1;

    for (int i = 0; i < servers_num; i++) {
        uint64_t numbers_for_this_server = numbers_per_server;
        if ((uint64_t)i < remainder) {
            numbers_for_this_server++;
        }

        thread_args[i].server = servers[i];
        thread_args[i].begin = current_start;
        thread_args[i].end = current_start + numbers_for_this_server - 1;
        thread_args[i].mod = mod;
        thread_args[i].result = 1;

        printf("Server %s:%d: numbers from %"PRIu64" to %"PRIu64"\n", 
               servers[i].ip, servers[i].port, thread_args[i].begin, thread_args[i].end);

        current_start += numbers_for_this_server;

        if (pthread_create(&threads[i], NULL, ClientThread, &thread_args[i]) != 0) {
            fprintf(stderr, "Error creating thread for server %s:%d\n", 
                    servers[i].ip, servers[i].port);
        }
    }

    uint64_t total_result = 1;
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        total_result = MultModulo(total_result, thread_args[i].result, mod);
    }

    printf("Final result: %"PRIu64"! mod %"PRIu64" = %"PRIu64"\n", k, mod, total_result);

    free(servers);
    return 0;
}