#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h> // Internet address family structures (sockaddr_in)
#include <netdb.h>      // For getaddrinfo() to resolve hostnames
#define FIELD_SIZE 10

int main(int t_narg, char **t_args)
{
    int l_port = atoi(t_args[2]);
    char *l_host = t_args[1];

    int sock;
    struct sockaddr_in server_addr;
    char playingField[FIELD_SIZE][FIELD_SIZE];
    if (!l_host || !l_port)
    {
        fprintf(stderr, "Host or port is missing!");
        exit(1);
    }
    fprintf(stderr, "Attempting to connect to '%s' on port %d.", l_host, l_port);
    // Resolve the server's address
    addrinfo l_ai_req, *l_ai_ans;
    bzero(&l_ai_req, sizeof(l_ai_req));
    l_ai_req.ai_family = AF_INET;       // Use IPv4
    l_ai_req.ai_socktype = SOCK_STREAM; // Use TCP
    // getaddrinfo resolves hostname to an address, results are in l_ai_ans
    if (getaddrinfo(l_host, nullptr, &l_ai_req, &l_ai_ans) != 0)
    {
        fprintf(stderr, "Unknown host name!");
        exit(1);
    }
    // Copy the resolved address information
    sockaddr_in l_cl_addr = *(sockaddr_in *)l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons(l_port); // Set the port in network byte order
    freeaddrinfo(l_ai_ans);             // Free the memory allocated by getaddrinfo

    // Создаём сокет
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(l_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Подключаемся к серверу
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    printf("Connected to server.\n");

    // Основной цикл: принимаем и отображаем поле
    while (1)
    {
        ssize_t bytes = recv(sock, playingField, sizeof(playingField), 0);
        if (bytes <= 0)
        {
            printf("Disconnected from server.\n");
            break;
        }

        // Отрисовка поля
        system("clear"); // Очистка консоли (для Windows — cls)
        printf("=== Playing Field ===\n");
        for (int i = 0; i < FIELD_SIZE; i++)
        {
            for (int j = 0; j < FIELD_SIZE; j++)
            {
                printf("%c ", playingField[i][j]);
            }
            printf("\n");
        }
        printf("=====================\n");

        // Можно ввести команду для отправки
        char input[32];
        printf("Enter move (i-j-symbol): ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        // Убираем \n
        input[strcspn(input, "\n")] = '\0';

        // Отправляем серверу
        send(sock, input, strlen(input), 0);
    }

    close(sock);
    return 0;
}
