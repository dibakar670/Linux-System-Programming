#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <time.h>

#define PORT 8080
#define SHM_NAME "/my_shm"
#define SHM_SIZE 1024
#define SEM_NAME "/my_sem"
#define MAX_CLIENTS_PER_THREAD 5

// Global variables
sem_t *semaphore;
char *shared_memory;

// Client structure
typedef struct {
    int clients[MAX_CLIENTS_PER_THREAD];
    int count;
} client_group_t;

// Function prototypes
void *generate_random_numbers(void *arg);
void *handle_clients(void *arg);
void cleanup();

int main() {
    // Socket variables
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Initialize shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SHM_SIZE);
    shared_memory = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Initialize semaphore
    semaphore = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    // Create socket file descriptor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // Listen for incoming connections
    listen(server_fd, 3);

    // Thread for generating random numbers
    pthread_t generator_thread;
    pthread_create(&generator_thread, NULL, generate_random_numbers, NULL);

    // Initial client handler thread
    client_group_t *group = malloc(sizeof(client_group_t));
    group->count = 0;
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, handle_clients, group);

    // Accept client connections
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        group->clients[group->count++] = new_socket;
        if (group->count >= MAX_CLIENTS_PER_THREAD) {
            group = malloc(sizeof(client_group_t));
            group->count = 0;
            pthread_create(&client_thread, NULL, handle_clients, group);
        }
    }

    cleanup();
    return 0;
}

// Function to generate random numbers
void *generate_random_numbers(void *arg) {
    while (1) {
        sleep(1);
        int number = rand() % 90 + 10;
        sem_wait(semaphore);
        snprintf(shared_memory, SHM_SIZE, "%d", number);
        sem_post(semaphore);
    }
    return NULL;
}

// Function to handle clients
void *handle_clients(void *arg) {
    client_group_t *group = (client_group_t *)arg;
    while (1) {
        for (int i = 0; i < group->count; i++) {
            if (group->clients[i] != -1) {
                sem_wait(semaphore);
                char message[1024];
                snprintf(message, sizeof(message), "Random Number: %s\n", shared_memory);
                send(group->clients[i], message, strlen(message), 0);
                sem_post(semaphore);
            }
        }
        sleep(1);
    }
    return NULL;
}

// Function to clean up resources
void cleanup() {
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
    close(server_fd);
}
