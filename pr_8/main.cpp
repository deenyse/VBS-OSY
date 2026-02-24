#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 5
#define STRING_LENGTH 100

char buffer[BUFFER_SIZE][STRING_LENGTH]; // буфер строк
int in = 0;                              // индекс для вставки (producer)
int out = 0;                             // индекс для извлечения (consumer)

sem_t mutex; // защита критической секции
sem_t empty; // количество свободных мест
sem_t full;  // количество занятых мест

void producer(char *item)
{
    sem_wait(&empty); // ждём свободное место
    sem_wait(&mutex); // входим в критическую секцию

    strncpy(buffer[in], item, STRING_LENGTH);
    in = (in + 1) % BUFFER_SIZE;
    printf("[PRODUCER] Produced: %s\n", item);

    sem_post(&mutex); // выход из критической секции
    sem_post(&full);  // увеличиваем количество элементов
}

void consumer(char *item)
{
    sem_wait(&full);  // ждём элемент
    sem_wait(&mutex); // критическая секция

    strncpy(item, buffer[out], STRING_LENGTH);
    out = (out + 1) % BUFFER_SIZE;
    printf("[CONSUMER] Consumed: %s\n", item);

    sem_post(&mutex); // выход
    sem_post(&empty); // увеличиваем количество свободных мест
}

void *client_handler(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    char task[9];
    read(client_socket, task, sizeof(task));

    if (strcmp(task, "producer") == 0)
    {
        printf("Producer is connected\n");
        char line[STRING_LENGTH];
        while (read(client_socket, line, sizeof(line)) > 0)
        {
            producer(line);
            write(client_socket, "OK\n", 3);
        }
    }
    else if (strcmp(task, "consumer") == 0)
    {
        printf("Consumer is connected\n");
        char line[STRING_LENGTH];
        while (1)
        {
            consumer(line);
            if (write(client_socket, line, strlen(line)) <= 0)
                break;
        }
    }
    close(client_socket);
    return NULL;
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    // Инициализация
    sem_init(&mutex, 0, 1);           // двоичный семафор
    sem_init(&empty, 0, BUFFER_SIZE); // свободных мест = размер буфера
    sem_init(&full, 0, 0);            // изначально буфер пуст

    printf("Server running on port 12345...\n");

    while (1)
    {
        int *client_socket = (int *)malloc(sizeof(int));
        *client_socket = accept(server_fd, NULL, NULL);
        pthread_t t;
        pthread_create(&t, NULL, client_handler, client_socket);
        pthread_detach(t);
    }
    return 0;
}