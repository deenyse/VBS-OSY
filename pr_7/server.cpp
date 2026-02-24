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

typedef struct
{
    int socket;
    char nick[32];
} Client;

Client clients[MAX_CLIENTS];
int num_clients = 0;

void broadcast(const char *msg, int exclude_sock)
{
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i].socket != exclude_sock)
        {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
}

void *handle_client(void *arg)
{
    int sock = *(int *)arg;
    free(arg);

    // Vyzvat k zadání nicku
    char prompt[] = "Enter your nick: ";
    send(sock, prompt, strlen(prompt), 0);

    char nick[32];
    memset(nick, 0, sizeof(nick));
    int len = recv(sock, nick, sizeof(nick) - 1, 0);
    if (len <= 0)
    {
        close(sock);
        pthread_exit(NULL);
    }
    nick[len] = '\0';

    // Přidat klienta do seznamu
    if (num_clients < MAX_CLIENTS)
    {
        clients[num_clients].socket = sock;
        strcpy(clients[num_clients].nick, nick);
        num_clients++;
    }
    else
    {
        // Pokud je seznam plný, uzavřít spojení
        close(sock);
        pthread_exit(NULL);
    }

    // Broadcast o připojení
    char join_msg[64];
    snprintf(join_msg, sizeof(join_msg), "%s has joined the chat.\n", nick);
    printf("%s", join_msg);
    broadcast(join_msg, sock);

    // Čtení zpráv od klienta
    char buf[BUFFER_SIZE];
    while ((len = recv(sock, buf, BUFFER_SIZE - 1, 0)) > 0)
    {
        buf[len] = '\0';

        // Vytvořit formátovanou zprávu
        char msg[BUFFER_SIZE + 64];
        snprintf(msg, sizeof(msg), "%s: %s\n", nick, buf);

        // Zobrazit na serveru
        printf("%s", msg);

        // Broadcast ostatním klientům
        broadcast(msg, sock);
    }

    // Odstranit klienta ze seznamu
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i].socket == sock)
        {
            memmove(&clients[i], &clients[i + 1], (num_clients - i - 1) * sizeof(Client));
            num_clients--;
            break;
        }
    }

    // Broadcast o odpojení
    snprintf(join_msg, sizeof(join_msg), "%s has left the chat.\n", nick);
    printf("%s", join_msg);
    broadcast(join_msg, -1);

    close(sock);
    pthread_exit(NULL);
}

void *console_handler(void *arg)
{
    char buf[BUFFER_SIZE];
    while (fgets(buf, BUFFER_SIZE, stdin) != NULL)
    {
        // Odstranit nový řádek
        buf[strcspn(buf, "\n")] = '\0';

        // Vytvořit zprávu od serveru
        char msg[BUFFER_SIZE + 64];
        snprintf(msg, sizeof(msg), "Server: %s\n", buf);

        // Broadcast všem klientům
        broadcast(msg, -1);
    }
    return NULL;
}

int main(int t_narg, char **t_args)
{
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

    // Vlákno pro vstup z konzole
    pthread_t console_thread;
    pthread_create(&console_thread, NULL, console_handler, NULL);

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