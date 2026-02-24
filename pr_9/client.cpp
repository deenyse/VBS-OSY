#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h> // Internet address family structures (sockaddr_in)
#include <netdb.h>      // For getaddrinfo() to resolve hostnames

#define STRING_LENGTH 100

void producer(int sock, FILE *f)
{
    // identify as producer
    write(sock, "producer\0", 9);

    char line[STRING_LENGTH];
    while (fgets(line, sizeof(line), f))
    {
        write(sock, line, strlen(line) + 1); // include null terminator

        char ok[4];
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
}

void consumer(int sock, FILE *f)
{
    // identify as consumer
    write(sock, "consumer\0", 9);

    char line[STRING_LENGTH];
    ssize_t n;

    while ((n = read(sock, line, sizeof(line) - 1)) > 0)
    {
        line[n] = '\0';
        printf("Consumed: %s\n", line);
        fprintf(f, "%s", line);
        fflush(f);

        write(sock, "OK\n", 3);
    }

    printf("Server closed connection.\n");
    close(sock);
}

int main(int argc, char **argv)
{
    int l_port = 0;
    char *l_host = nullptr;
    char *l_role = nullptr;
    char *l_filename = nullptr;

    l_host = argv[1];
    l_port = atoi(argv[2]);
    l_role = argv[3];
    l_filename = argv[4];

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

    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        fprintf(stderr, "Unable to create a socket.");
        exit(1);
    }

    // Connect to the server
    if (connect(sock, (sockaddr *)&l_cl_addr, sizeof(l_cl_addr)) < 0)
    {
        fprintf(stderr, "Unable to connect to the server.");
        exit(1);
    }

    char task[6];
    ssize_t n = read(sock, task, sizeof(task) - 1);

    FILE * f = nullptr;
    if (strncmp(l_role, "producer", 8) == 0)
    {
        f = fopen(l_filename, "r");
        if (!f)
        {
            perror("fopen");
            close(sock);
            exit(EXIT_FAILURE);
        }
        producer(sock, f);
    }
    if (strncmp(l_role, "consumer", 8) == 0)
    {
        f = fopen(l_filename, "w");
        if (!f)
        {
            perror("fopen");
            close(sock);
            exit(EXIT_FAILURE);
        }
        consumer(sock , f);

    }


    return 0;
}
