#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *host = argv[1];
    int port = atoi(argv[2]);

    // --- Resolve hostname ---
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[10];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // --- Create socket ---
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
    {
        perror("socket");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // --- Connect ---
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("connect");
        close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s:%d\n", host, port);
    freeaddrinfo(res);

    // --- Set up poll for socket and stdin ---
    struct pollfd fds[2];
    fds[0].fd = sock;
    fds[0].events = POLLIN; // watch for server messages

    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN; // watch for keyboard input

    char buffer[BUFFER_SIZE];

    // --- Main loop ---
    while (1)
    {
        int ret = poll(fds, 2, -1); // wait indefinitely until something happens
        if (ret < 0)
        {
            perror("poll");
            break;
        }

        // --- If there is data from the server ---
        if (fds[0].revents & POLLIN)
        {
            ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0)
            {
                printf("Server disconnected.\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer);
            fflush(stdout);
        }

        // --- If user typed something ---
        if (fds[1].revents & POLLIN)
        {
            if (fgets(buffer, sizeof(buffer), stdin) == NULL)
                break;

            buffer[strcspn(buffer, "\n")] = '\0'; // remove newline

            if (strlen(buffer) > 0)
            {
                if (send(sock, buffer, strlen(buffer), 0) < 0)
                {
                    perror("send");
                    break;
                }
            }
        }
    }

    close(sock);
    return 0;
}
