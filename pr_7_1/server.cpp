#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 256

int clients[MAX_CLIENTS];
int num_clients = 0;

#define FIELD_SIZE 10
char playingField[FIELD_SIZE][FIELD_SIZE];

void broadcast(int exclude_sock)
{
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i] != exclude_sock)
        {
            send(clients[i], playingField, sizeof(char) * FIELD_SIZE * FIELD_SIZE, 0);
        }
    }
}

void *handle_client(void *arg)
{
    int sock = *(int *)arg;
    free(arg);

    // Přidat klienta do seznamu
    if (num_clients < MAX_CLIENTS)
    {
        clients[num_clients] = sock;
        num_clients++;
    }
    else
    {
        // Pokud je seznam plný, uzavřít spojení
        close(sock);
        pthread_exit(NULL);
    }

    send(sock, playingField, sizeof(char) * FIELD_SIZE * FIELD_SIZE, 0);

    // Čtení zpráv od klienta
    char buf[BUFFER_SIZE];
    int len;
    while ((len = recv(sock, buf, BUFFER_SIZE - 1, 0)) > 0)
    {
        buf[len] = '\0';
        //
        // HERE BROADCAST CHANGES
        char *firstSeparator = strchr(buf, '-');
        *firstSeparator = '\0';

        char *secondSeparator = strchr(firstSeparator + 1, '-');
        *secondSeparator = '\0';

        int i = atoi(buf);
        if (i < 0 || i >= FIELD_SIZE)
        {
            fprintf(stderr, "i is out of range\n");
            continue;
        }
        int j = atoi(firstSeparator + 1);
        if (j < 0 || j >= FIELD_SIZE)
        {
            fprintf(stderr, "j is out of range\n");
            continue;
        }
        char symbol = *(secondSeparator + 1);
        if (symbol < 'a' || symbol > 'z')
        {
            fprintf(stderr, "symbol is out of range\n");
            continue;
        }

        playingField[i][j] = symbol;

        broadcast(-1);
    }

    // Odstranit klienta ze seznamu
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i] == sock)
        {
            memmove(&clients[i], &clients[i + 1], (num_clients - i - 1) * sizeof(int));
            num_clients--;
            break;
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int t_narg, char **t_args)
{
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            playingField[i][j] = ' ';
        }
    }

    int l_port = atoi(t_args[1]);

    int server_sock, client_sock;
    struct sockaddr_in server_addr;
    socklen_t addr_size;

    // Vytvoření serverového soketu
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(l_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_sock, 5) != 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", l_port);

    // // Vlákno pro vstup z konzole
    // pthread_t console_thread;
    // pthread_create(&console_thread, NULL, console_handler, NULL);

    // Přijímání klientů
    while (1)
    {
        addr_size = sizeof(struct sockaddr_in);
        client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0)
        {
            perror("Accept failed");
            continue;
        }

        int *arg = (int *)malloc(sizeof(int));
        if (arg == NULL)
        {
            close(client_sock);
            continue;
        }
        *arg = client_sock;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, arg) != 0)
        {
            perror("Thread creation failed");
            free(arg);
            close(client_sock);
        }
        pthread_detach(thread);
    }

    close(server_sock);
    return 0;
}