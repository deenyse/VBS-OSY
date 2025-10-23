//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// multiple clients using fork().
// The mandatory argument of program is port number for listening.
//
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h> // for waitpid
#include <vector>
#include <string>

#define STR_CLOSE "close"
#define STR_QUIT "quit"

//***************************************************************************
// log messages

#define LOG_ERROR 0 // errors
#define LOG_INFO 1  // information and notifications
#define LOG_DEBUG 2 // debug messages

// debug flag
int g_debug = LOG_INFO;

void log_msg(int t_log_level, const char *t_form, ...)
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"};

    if (t_log_level && t_log_level > g_debug)
        return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsprintf(l_buf, t_form, l_arg);
    va_end(l_arg);

    switch (t_log_level)
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf(stdout, out_fmt[t_log_level], l_buf);
        break;

    case LOG_ERROR:
        fprintf(stderr, out_fmt[t_log_level], errno, strerror(errno), l_buf);
        break;
    }
}

//***************************************************************************
// help

void help(int t_narg, char **t_args)
{
    if (t_narg <= 1 || !strcmp(t_args[1], "-h"))
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n",
            t_args[0]);

        exit(0);
    }

    if (!strcmp(t_args[1], "-d"))
        g_debug = LOG_DEBUG;
}

//***************************************************************************
// Function to process a batch of expressions with Python
void process_batch(int t_sock_client, std::vector<std::string> &expressions)
{
    if (expressions.empty())
        return;

    std::string py_cmd;
    for (const auto &expr : expressions)
    {
        py_cmd += "print(\"" + expr + "=\", " + expr + ")\n";
    }

    // Create pipe for Python stdin
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        log_msg(LOG_ERROR, "Unable to create pipe.");
        return;
    }

    // Fork grandchild for Python execution
    pid_t py_pid = fork();
    if (py_pid < 0)
    {
        log_msg(LOG_ERROR, "Fork for Python failed!");
        close(pipe_fd[0]); // read
        close(pipe_fd[1]); // write
        return;
    }

    if (py_pid == 0)
    {
        // Grandchild process: exec python3
        close(pipe_fd[1]);              // Close write end
        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin from pipe
        close(pipe_fd[0]);

        dup2(t_sock_client, STDOUT_FILENO); // Redirect stdout to client socket
        close(t_sock_client);               // Close original socket fd

        // Exec python3
        execlp("python3", "python3", NULL);
        log_msg(LOG_ERROR, "Exec python3 failed!");
        exit(1);
    }

    // Child process (handler): write to pipe and wait
    close(pipe_fd[0]); // Close read end
    write(pipe_fd[1], py_cmd.c_str(), py_cmd.length());
    close(pipe_fd[1]);

    // Wait for grandchild to finish to avoid zombies
    waitpid(py_pid, NULL, 0);

    expressions.clear();
}

void process_resize(int t_sock_client, std::string &resolution)
{
    if (resolution.empty())
        return;

    std::string resize_ag = resolution + "!";
    pid_t pid = fork();
    if (pid < 0)
    {
        log_msg(LOG_ERROR, "Fork for convert failed!");
        return;
    }

    if (pid == 0)
    {
        dup2(t_sock_client, STDOUT_FILENO);
        close(t_sock_client);

        execlp("convert", "convert", "-resize", resize_ag.c_str(), "babi-leto.png", "-", NULL);
        log_msg(LOG_ERROR, "Exec convert failed");
        exit(1);
    }

    waitpid(pid, NULL, 0);
    close(t_sock_client);
}

//***************************************************************************
// Function to handle communication with a single client (child process)
void handle_client(int t_sock_client)
{
    log_msg(LOG_INFO, "Child process handling client.");

    // Poll structure for client socket only (no stdin, as messages from server can't be sent)
    pollfd l_read_poll[1];
    l_read_poll[0].fd = t_sock_client;
    l_read_poll[0].events = POLLIN;
    std::vector<std::string> expressions;

    while (1)
    {
        char l_buf[256];

        // Poll for data from client
        int l_poll = poll(l_read_poll, 1, -1);

        if (l_poll < 0)
        {
            log_msg(LOG_ERROR, "Function poll failed in child!");
            close(t_sock_client);
            exit(1);
        }

        // Data from client?
        if (l_read_poll[0].revents & POLLIN)
        {
            // Read data from socket
            // l_len - number of bytes
            int l_len = read(t_sock_client, l_buf, sizeof(l_buf));
            if (l_len == 0)
            {
                log_msg(LOG_DEBUG, "Client closed socket!");
                close(t_sock_client);
                exit(0);
            }
            if (l_len < 0)
            {
                log_msg(LOG_ERROR, "Unable to read data from client.");
                close(t_sock_client);
                exit(1);
            }

            l_buf[l_len] = '\0';
            log_msg(LOG_DEBUG, "Read %d bytes from client: %s", l_len, l_buf);

            // strchr - search in l_buf the "\n\"
            char *newline = strchr(l_buf, '\n');
            if (newline)
                *newline = '\0';

            std::string resolution(l_buf);

            // Write data to stdout
            // l_len = write(STDOUT_FILENO, l_buf, l_len);

            // Close request?
            if (!strncasecmp(l_buf, STR_CLOSE, strlen(STR_CLOSE)))
            {
                log_msg(LOG_INFO, "Client sent 'close' request to close connection.");
                close(t_sock_client);
                log_msg(LOG_INFO, "Connection closed.");
                exit(0);
            }

            process_resize(t_sock_client, resolution);
        }
    }
}

//***************************************************************************

int main(int t_narg, char **t_args)
{
    if (t_narg <= 1)
        help(t_narg, t_args);

    int l_port = 0;

    // parsing arguments
    for (int i = 1; i < t_narg; i++)
    {
        if (!strcmp(t_args[i], "-d"))
            g_debug = LOG_DEBUG;

        if (!strcmp(t_args[i], "-h"))
            help(t_narg, t_args);

        if (*t_args[i] != '-' && !l_port)
        {
            l_port = atoi(t_args[i]);
            break;
        }
    }

    if (l_port <= 0)
    {
        log_msg(LOG_INFO, "Bad or missing port number %d!", l_port);
        help(t_narg, t_args);
    }

    log_msg(LOG_INFO, "Server will listen on port: %d.", l_port);

    // socket creation
    int l_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_listen == -1)
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    // INADDR_ANY - 0.0.0.0 (это значит любой адресс может подключится)
    in_addr l_addr_any = {INADDR_ANY};
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons(l_port);
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    // setsockopt - устанавливает опций сокета
    // SO_REUSEADDR - позволяет повторно использовать адрес и потр
    if (setsockopt(l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0)
        log_msg(LOG_ERROR, "Unable to set socket option!");

    // assign port number to socket
    if (bind(l_sock_listen, (const sockaddr *)&l_srv_addr, sizeof(l_srv_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Bind failed!");
        close(l_sock_listen);
        exit(1);
    }

    // listenig on set port
    if (listen(l_sock_listen, 5) < 0) // Increased backlog to 5 for multiple clients
    {
        log_msg(LOG_ERROR, "Unable to listen on given port!");
        close(l_sock_listen);
        exit(1);
    }

    log_msg(LOG_INFO, "Enter 'quit' to quit server.");

    // go!
    while (1)
    {
        // list of fd sources for parent: stdin and listen socket
        pollfd l_read_poll[2];

        l_read_poll[0].fd = STDIN_FILENO;
        l_read_poll[0].events = POLLIN;
        l_read_poll[1].fd = l_sock_listen;
        l_read_poll[1].events = POLLIN;

        // select from fds
        int l_poll = poll(l_read_poll, 2, -1);

        if (l_poll < 0)
        {
            log_msg(LOG_ERROR, "Function poll failed!");
            exit(1);
        }

        if (l_read_poll[0].revents & POLLIN)
        { // data on stdin
            char buf[128];

            int l_len = read(STDIN_FILENO, buf, sizeof(buf));
            if (l_len == 0)
            {
                log_msg(LOG_DEBUG, "Stdin closed.");
                exit(0);
            }
            if (l_len < 0)
            {
                log_msg(LOG_DEBUG, "Unable to read from stdin!");
                exit(1);
            }

            log_msg(LOG_DEBUG, "Read %d bytes from stdin", l_len);
            // request to quit?
            if (!strncmp(buf, STR_QUIT, strlen(STR_QUIT)))
            {
                log_msg(LOG_INFO, "Request to 'quit' entered.");
                close(l_sock_listen);
                // Optionally wait for children, but for simplicity, exit
                exit(0);
            }
        }

        if (l_read_poll[1].revents & POLLIN)
        {                      // new client?
            sockaddr_in l_rsa; // client information
            int l_rsa_size = sizeof(l_rsa);
            // new connection
            int l_sock_client = accept(l_sock_listen, (sockaddr *)&l_rsa, (socklen_t *)&l_rsa_size);
            if (l_sock_client == -1)
            {
                log_msg(LOG_ERROR, "Unable to accept new client.");
                continue; // Don't exit, continue listening
            }

            uint l_lsa = sizeof(l_srv_addr);
            // my IP
            getsockname(l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa);
            log_msg(LOG_INFO, "My IP: '%s'  port: %d",
                    inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));
            // client IP
            getpeername(l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa);
            log_msg(LOG_INFO, "Client IP: '%s'  port: %d",
                    inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));

            // Fork a child process to handle the client
            pid_t l_pid = fork();
            if (l_pid < 0)
            {
                log_msg(LOG_ERROR, "Fork failed!");
                close(l_sock_client);
                continue;
            }

            if (l_pid == 0)
            {
                // Child process
                close(l_sock_listen); // Child doesn't need listen socket
                handle_client(l_sock_client);
                // handle_client exits on its own
            }
            else
            {
                // Parent process
                close(l_sock_client); // Parent doesn't need client socket
                log_msg(LOG_INFO, "Forked child PID %d to handle client.", l_pid);
                // Optionally reap children to avoid zombies
                waitpid(-1, NULL, WNOHANG);
            }
        }
    } // while (1)

    return 0;
}
