#include <iostream>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <poll.h>
#include <mqueue.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

#define MQ_NAME "/chat_queue_cpp"
#define MSG_TEXT_SIZE 128

struct msg_t
{
    pid_t sender_pid;
    int message_type; // 0 = zpráva, 1 = nick
    char text[MSG_TEXT_SIZE];
};

struct Client
{
    int sockfd;
    std::string nick;
};

std::map<pid_t, Client> clients;

void remove_client(pid_t pid)
{
    auto it = clients.find(pid);
    if (it != clients.end())
    {
        close(it->second.sockfd);
        clients.erase(it);
    }
}

void broadcast_from_pid(pid_t sender, const std::string &text)
{
    std::string nick = "unknown";
    auto it = clients.find(sender);
    if (it != clients.end() && !it->second.nick.empty())
        nick = it->second.nick;

    std::string formatted = "[" + nick + "]: " + text + "\n";

    for (auto &[pid, client] : clients)
    {
        if (pid != sender)
            send(client.sockfd, formatted.c_str(), formatted.size(), MSG_NOSIGNAL);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(port);

    if (bind(listenfd, (sockaddr *)&sa, sizeof(sa)) < 0)
    {
        perror("bind");
        return 1;
    }
    if (listen(listenfd, 16) < 0)
    {
        perror("listen");
        return 1;
    }

    // --- POSIX MQ ---
    mq_unlink(MQ_NAME);
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = 0;
    attr.mq_maxmsg = 16;
    attr.mq_msgsize = sizeof(msg_t);
    attr.mq_curmsgs = 0;

    mqd_t mqdes = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mqdes == (mqd_t)-1)
    {
        perror("mq_open parent");
        return 1;
    }

    std::cout << "Server listening on port " << port << "\n";

    struct pollfd fds[2];
    fds[0].fd = listenfd;
    fds[0].events = POLLIN;
    fds[1].fd = (int)mqdes;
    fds[1].events = POLLIN;

    while (true)
    {
        // Проверка завершившихся детей
        int status;
        pid_t w;
        while ((w = waitpid(-1, &status, WNOHANG)) > 0)
            remove_client(w);

        int ret = poll(fds, 2, 500);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        // Новое соединение
        if (fds[0].revents & POLLIN)
        {
            int conn = accept(listenfd, nullptr, nullptr);
            if (conn < 0)
                continue;

            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork");
                close(conn);
                continue;
            }

            else if (pid == 0)
            {
                // -------- CHILD --------
                close(listenfd);

                mqd_t mqw = mq_open(MQ_NAME, O_WRONLY);
                if (mqw == (mqd_t)-1)
                {
                    perror("child mq_open");
                    close(conn);
                    _exit(1);
                }

                // запрос ника
                std::string prompt = "Enter your nick: ";
                send(conn, prompt.c_str(), prompt.size(), 0);

                char buf[MSG_TEXT_SIZE];
                ssize_t n = recv(conn, buf, sizeof(buf) - 1, 0);
                if (n <= 0)
                {
                    mq_close(mqw);
                    close(conn);
                    _exit(0);
                }
                buf[n] = 0;

                // регистрация ника
                msg_t reg{};
                reg.sender_pid = getpid();
                reg.message_type = 1;
                strncpy(reg.text, buf, sizeof(reg.text) - 1);
                mq_send(mqw, (char *)&reg, sizeof(reg), 0);

                // цикл сообщений
                while (true)
                {
                    ssize_t r = recv(conn, buf, sizeof(buf) - 1, 0);
                    if (r <= 0)
                        break;
                    buf[r] = 0;

                    msg_t m{};
                    m.sender_pid = getpid();
                    m.message_type = 0;
                    strncpy(m.text, buf, sizeof(m.text) - 1);
                    mq_send(mqw, (char *)&m, sizeof(m), 0);
                }

                mq_close(mqw);
                close(conn);
                _exit(0);
            }
            else
            {
                // -------- PARENT --------
                clients[pid] = {conn, ""};
                std::cout << "Accepted client pid=" << pid << "\n";
            }
        }

        // сообщения из очереди
        if (fds[1].revents & POLLIN)
        {
            while (true)
            {
                msg_t m;
                ssize_t r = mq_receive(mqdes, (char *)&m, sizeof(m), nullptr);
                if (r < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    perror("mq_receive");
                    break;
                }

                if (m.message_type == 1)
                {
                    auto it = clients.find(m.sender_pid);
                    if (it != clients.end())
                        it->second.nick = m.text;
                    std::cout << "Registered pid " << m.sender_pid << " as '" << m.text << "'\n";
                }
                else if (m.message_type == 0)
                {
                    broadcast_from_pid(m.sender_pid, m.text);
                }
            }
        }
    }

    mq_close(mqdes);
    mq_unlink(MQ_NAME);
    close(listenfd);
    return 0;
}
