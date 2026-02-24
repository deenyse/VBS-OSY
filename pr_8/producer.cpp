#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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

    // identify as producer
    write(sock, "producer", strlen("producer") + 1);

    FILE *f = fopen("jmena.txt", "r");
    if (!f)
    {
        perror("fopen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char line[STRING_LENGTH];
    while (fgets(line, sizeof(line), f))
    {
        line[strcspn(line, "\n")] = 0;       // remove newline
        write(sock, line, strlen(line) + 1); // include null terminator

        char ok[10];
        ssize_t n = read(sock, ok, sizeof(ok) - 1);
        if (n > 0)
        {
            ok[n] = '\0';
            printf("[CLIENT] Server response: %s", ok);
        }
        else
        {
            break;
        }
    }

    fclose(f);
    close(sock);
    return 0;
}
