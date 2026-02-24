#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define STRING_LENGTH 100

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // identify as consumer
    write(sock, "consumer", strlen("consumer") + 1);

    char line[STRING_LENGTH];
    ssize_t n;

    while ((n = read(sock, line, sizeof(line) - 1)) > 0)
    {
        line[n] = '\0';
        printf("[CLIENT] Got: %s\n", line);
        write(sock, "OK\n", 3);
    }

    printf("Server closed connection.\n");
    close(sock);
    return 0;
}
