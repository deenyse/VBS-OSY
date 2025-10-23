#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <errno.h>

class Client
{
private:
    int sock;
    struct sockaddr_in server_addr;

public:
    Client(const char *ip, int port)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            fprintf(stderr, "Ошибка: не удалось создать сокет: %s\n", strerror(errno));
            return;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip);
    }

    int connect_to_server()
    {
        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            fprintf(stderr, "Ошибка: не удалось подключиться к серверу: %s\n", strerror(errno));
            return -1;
        }
        fprintf(stdout, "Подключено к серверу %s:%d\n",
                inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
        return 0;
    }

    int send_message(const char *message)
    {
        std::string msg = std::string(message) + "\n"; // Добавляем \n к каждому сообщению
        if (send(sock, msg.c_str(), msg.length(), 0) < 0)
        {
            fprintf(stderr, "Ошибка: не удалось отправить сообщение: %s\n", strerror(errno));
            return -1;
        }
        return 0;
    }

    int receive_message(char *buffer, size_t size)
    {
        int bytes = recv(sock, buffer, size - 1, 0);
        if (bytes <= 0)
        {
            if (bytes == 0)
                fprintf(stderr, "Сервер отключился\n");
            else
                fprintf(stderr, "Ошибка: не удалось получить ответ: %s\n", strerror(errno));
            return -1;
        }

        buffer[bytes] = '\0';
        return bytes;
    }

    void run()
    {
        char input[256];
        char response[1024]; // чуть больше буфера на вывод нескольких выражений
        std::vector<std::string> batch;

        const int BATCH_SIZE = 3;

        while (true)
        {
            if (!fgets(input, sizeof(input), stdin))
            {
                fprintf(stderr, "Ошибка ввода\n");
                break;
            }

            // Убираем перенос строки
            size_t len = strlen(input);
            if (len > 0 && (input[len - 1] == '\n' || input[len - 1] == '\r'))
                input[len - 1] = '\0';

            if (strcmp(input, "exit") == 0)
                break;

            if (strlen(input) == 0)
                continue; // игнорируем пустые строки

            batch.push_back(input);

            if (batch.size() == BATCH_SIZE)
            {
                // Формируем один Python print с переносами
                std::string pycmd = "";
                for (size_t i = 0; i < batch.size(); ++i)
                {
                    pycmd += "\"" + batch[i] + "=\", " + batch[i];
                    if (i != batch.size() - 1)
                        pycmd += ", \"\\n\", ";
                }

                // Отправляем на сервер
                if (send_message(pycmd.c_str()) < 0)
                    break;

                // Получаем ответ от сервера
                int bytes = receive_message(response, sizeof(response));
                if (bytes <= 0)
                    break;

                // Выводим результат как есть
                printf("%s", response);

                batch.clear();
            }
        }

        close_connection();
    }

    void close_connection()
    {
        close(sock);
        fprintf(stdout, "Соединение закрыто\n");
    }

    ~Client()
    {
        close(sock);
    }
};

int main()
{
    Client client("127.0.0.1", 12345);
    if (client.connect_to_server() == 0)
    {
        client.run();
    }
    return 0;
}