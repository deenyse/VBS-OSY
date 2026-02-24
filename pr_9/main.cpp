#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define BUFFER_SIZE 5
#define STRING_LENGTH 100

#define SHM_NAME "/shared_buffer"
#define SEM_MUTEX "/sem_mutex"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"

struct SharedBuffer
{
    char data[BUFFER_SIZE][STRING_LENGTH];
    int head;
    int tail;
};

struct SharedBuffer *shm;
sem_t *mutex, *empty, *full;

// Add an item to the circular buffer
void produce(char *item)
{
    sem_wait(empty);
    sem_wait(mutex);

    strncpy(shm->data[shm->head], item, STRING_LENGTH);
    shm->head = (shm->head + 1) % BUFFER_SIZE;
    printf("[PRODUCER] Produced -> %s\n", item);

    sem_post(mutex);
    sem_post(full);
}

// Remove an item from the circular buffer
void consume(char *item)
{
    sem_wait(full);
    sem_wait(mutex);

    strncpy(item, shm->data[shm->tail], STRING_LENGTH);
    shm->tail = (shm->tail + 1) % BUFFER_SIZE;
    printf("[CONSUMER] Consumed -> %s\n", item);

    sem_post(mutex);
    sem_post(empty);
}

// Handles client connection
void handle_client(int client_socket)
{
    write(client_socket, "Task?\n", 6);

    char role[16];
    read(client_socket, role, sizeof(role));

    char line[STRING_LENGTH];
    if (strstr(role, "producer"))
    {
        printf("Producer connected\n");
        while (read(client_socket, line, sizeof(line)) > 0)
        {
            produce(line);
            write(client_socket, "OK\n", 3);
        }
    }
    else if (strstr(role, "consumer"))
    {
        printf("Consumer connected\n");
        while (1)
        {
            consume(line);
            write(client_socket, line, strlen(line));
        }
    }
    close(client_socket);
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Remove old shared memory and semaphores if they exist
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);

    // Create and map shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(struct SharedBuffer));
    shm = (struct SharedBuffer *)mmap(NULL, sizeof(struct SharedBuffer),
                                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // POSIX Shared Memory (IPC) usage:
    //
    // 1. Create or open a shared memory segment:
    //    int shm_fd = shm_open("/shm_name", O_CREAT | O_RDWR, 0666);
    //      - O_CREAT -> create if it doesn't exist
    //      - O_RDWR  -> read/write access for this process
    //
    // 2. Set the size of the segment:
    //    ftruncate(shm_fd, sizeof(struct SharedBuffer));
    //
    // 3. Map it into process memory:
    //    shm = (struct SharedBuffer *)mmap(NULL, sizeof(struct SharedBuffer),
    //                                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    //      - PROT_READ  -> allow reading
    //      - PROT_WRITE -> allow writing
    //      - MAP_SHARED -> changes visible to all processes mapping this memory
    //      - MAP_PRIVATE -> local copy, changes not visible to others
    //
    // 4. Potential pitfalls:
    //      - Forgetting ftruncate -> cannot write
    //      - Forgetting MAP_SHARED -> changes not visible to other processes
    //      - Not checking shm_open or mmap return values
    //      - Not calling shm_unlink() after use -> segment persists in /dev/shm

    shm->head = 0;
    shm->tail = 0;

    // Create named semaphores
    mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    empty = sem_open(SEM_EMPTY, O_CREAT, 0666, BUFFER_SIZE);
    full = sem_open(SEM_FULL, O_CREAT, 0666, 0);

    // POSIX permissions (octal) for shared memory / semaphores:
    // Owner / Group / Others (r=4, w=2, x=1)
    //
    // 0666 -> rw-rw-rw-  : read/write for everyone
    // 0644 -> rw-r--r--  : owner read/write, others read
    // 0600 -> rw-------  : owner read/write only
    // 0444 -> r--r--r--  : read-only for everyone
    // 0222 -> -w--w--w-  : write-only for everyone
    // 0402 -> r---w---w- : owner read, others write (rare / not practical)
    //
    // Usage:
    // shm_open("/name", O_CREAT | O_RDWR, 0666);  // full read/write
    // shm_open("/name", O_CREAT | O_RDONLY, 0444); // read-only
    // sem_open("/sem", O_CREAT, 0666, 1);          // binary semaphore (mutex)
    // sem_open("/sem", O_CREAT, 0666, N);          // counting semaphore, initial value = N

    int port = atoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Server running on port %d\n", port);

    while (1)
    {
        int client_socket = accept(server_fd, NULL, NULL);
        if (client_socket < 0)
            continue;

        pid_t pid = fork();
        if (pid == 0)
        { // Child process for each client
            close(server_fd);
            handle_client(client_socket);
        }
        else
        {
            close(client_socket);
        }
    }

    return 0;
}
