#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <server-ip> <port>\n";
        return 1;
    }

    std::string ip = argv[1];
    int port = std::stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);

    if (connect(sock, (sockaddr *)&sa, sizeof(sa)) < 0)
    {
        perror("connect");
        return 1;
    }

    char buf[128];
    ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0)
    {
        std::cout << "Server closed\n";
        return 0;
    }
    buf[n] = 0;
    std::cout << buf;

    std::string nick;
    std::getline(std::cin, nick);
    send(sock, nick.c_str(), nick.size(), 0);

    pid_t pid = fork();
    if (pid == 0)
    {
        // child: читать сервер
        while (true)
        {
            ssize_t r = recv(sock, buf, sizeof(buf) - 1, 0);
            if (r <= 0)
                break;
            buf[r] = 0;
            std::cout << "\r" << buf << "\nYou: ";
            std::cout.flush();
        }
        close(sock);
        _exit(0);
    }
    else
    {
        // parent: писать серверу
        std::string line;
        while (true)
        {
            std::cout << "You: ";
            if (!std::getline(std::cin, line))
                break;
            send(sock, line.c_str(), line.size(), 0);
        }
        close(sock);
        wait(nullptr);
    }

    return 0;
}
