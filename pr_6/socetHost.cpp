#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <string>
class Server
{
private:
    int server_socket;
    struct sockaddr_in server_addr;

    static void reap_zombie(int)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ; // чистим зомби-процессы
    }

public:
    Server(const char *ip, int port)
    {
        // Создаём сокет
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1)
        {
            fprintf(stderr, "Ошибка: не удалось создать сокет\n");
            return;
        }

        // Разрешаем повторное использование адреса (иначе bind может выдавать ошибку)
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Настраиваем адрес
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip);

        // Обработка SIGCHLD (чтобы не копились зомби)
        signal(SIGCHLD, reap_zombie);
    }

    int bind_and_listen()
    {
        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            fprintf(stderr, "Ошибка: bind()\n");
            close(server_socket);
            return -1;
        }

        if (listen(server_socket, 5) < 0)
        {
            fprintf(stderr, "Ошибка: listen()\n");
            close(server_socket);
            return -1;
        }

        fprintf(stdout, "Сервер слушает %s:%d\n",
                inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
        return 0;
    }

    void handle_client(int client_socket)
    {
        char buffer[256];
        std::string leftover;

        while (true)
        {
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0)
                break;

            buffer[bytes_received] = '\0';
            leftover += buffer;

            size_t pos;
            while ((pos = leftover.find('\n')) != std::string::npos)
            {
                std::string expr = leftover.substr(0, pos);
                leftover.erase(0, pos + 1);

                if (expr.empty())
                    continue;

                // Создаем пайп для stdin Python
                int pipe_stdin[2];
                pipe(pipe_stdin);

                pid_t pid = fork();
                if (pid == 0)
                {
                    // Дочерний процесс
                    close(pipe_stdin[1]); // Закрываем write-end для дочери

                    // Перенаправляем stdin Python на read-end
                    dup2(pipe_stdin[0], STDIN_FILENO);
                    dup2(client_socket, STDOUT_FILENO); // stdout Python -> сокет
                    dup2(client_socket, STDERR_FILENO); // stderr Python -> сокет

                    // Выполняем python3
                    char *args[] = {(char *)"python3", (char *)"-u", (char *)"-", NULL};
                    execvp("python3", args);
                    _exit(1);
                }
                else
                {
                    // Родительский процесс
                    close(pipe_stdin[0]); // Закрываем read-end родителю

                    // Формируем команду Python и пишем в stdin дочери
                    std::string pycmd = "print( " + expr + " )\n";
                    write(pipe_stdin[1], pycmd.c_str(), pycmd.size());
                    close(pipe_stdin[1]); // Закрываем stdin -> дочерний Python выполнит команду

                    waitpid(pid, NULL, 0); // Ждем завершения дочери
                }
            }
        }

        close(client_socket);
    }

    void run()
    {
        while (1)
        {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket < 0)
            {
                fprintf(stderr, "Ошибка: accept()\n");
                continue;
            }

            fprintf(stdout, "Новое соединение от %s:%d\n",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            pid_t pid = fork();

            if (pid < 0)
            {
                fprintf(stderr, "Ошибка: fork()\n");
                close(client_socket);
                continue;
            }

            if (pid == 0)
            {
                // Дочерний процесс
                close(server_socket); // дочке серверный сокет не нужен
                handle_client(client_socket);
                _exit(0);
            }
            else
            {
                // Родительский процесс
                close(client_socket); // родителю клиентский сокет не нужен
            }
        }
    }

    void close_server()
    {
        close(server_socket);
        fprintf(stdout, "Сервер остановлен\n");
    }

    ~Server()
    {
        close(server_socket);
    }
};

int main()
{
    Server server("127.0.0.1", 12345);

    if (server.bind_and_listen() == 0)
    {
        server.run();
    }

    server.close_server();
    return 0;
}
