//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// This program is example of socket client.
// The mandatory arguments of program is IP adress or name of server and
// a port number.
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
#include <netdb.h>
#include <sys/wait.h>

#define STR_CLOSE "close"

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
    va_start(l_arg, t_form);                        // Указывает что переменные аргмументы ("...") начинаются после t_form
    vsnprintf(l_buf, sizeof(l_buf), t_form, l_arg); // Сохраняем в l_buf отформатированную строку с t_form и "..."
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
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number\n"
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

int main(int t_narg, char **t_args)
{

    if (t_narg <= 2)
        help(t_narg, t_args);

    int l_port = 0;
    char *l_host = nullptr;
    char *resolution = nullptr;

    // parsing arguments
    for (int i = 1; i < t_narg; i++)
    {
        if (!strcmp(t_args[i], "-d"))
            g_debug = LOG_DEBUG;

        if (!strcmp(t_args[i], "-h"))
            help(t_narg, t_args);

        if (*t_args[i] != '-')
        {
            if (!l_host)
                l_host = t_args[i];
            else if (!l_port)
                l_port = atoi(t_args[i]);
            else if (!resolution)
                resolution = t_args[i];
        }
    }

    if (!l_host || !l_port || !resolution)
    {
        log_msg(LOG_INFO, "Host or port or resolution is missing!");
        help(t_narg, t_args);
        exit(1);
    }

    log_msg(LOG_INFO, "Connection to '%s':%d.", l_host, l_port);

    /*
    struct addrinfo {
        int              ai_family;    // Семейство адресов (например, AF_INET для IPv4)
        int              ai_socktype;  // Тип сокета (например, SOCK_STREAM для TCP)
        int              ai_protocol;  // Протокол
        socklen_t        ai_addrlen;   // Длина адреса
        struct sockaddr *ai_addr;      // Указатель на структуру sockaddr
        char            *ai_canonname; // Каноническое имя хоста
        struct addrinfo *ai_next;      // Указатель на следующий элемент списка
    };
     */
    addrinfo l_ai_req, *l_ai_ans;
    bzero(&l_ai_req, sizeof(l_ai_req)); // bzero заполняет всю структуру l_ai_req нулями
    l_ai_req.ai_family = AF_INET;       // IPv4
    l_ai_req.ai_socktype = SOCK_STREAM; // TCP

    // В l_get_ai хранится код возврата (0 - success, other - fail)
    int l_get_ai = getaddrinfo(l_host, nullptr, &l_ai_req, &l_ai_ans); // Результаты записываются в l_ai_ans
    if (l_get_ai)
    {
        log_msg(LOG_ERROR, "Unknown host name!");
        exit(1);
    }

    // l_ai_ans->ai_addr это pointer на структуру sockaddr
    /*
    struct sockaddr_in {
        sa_family_t    sin_family;  // Семейство адресов (AF_INET для IPv4)
        in_port_t      sin_port;    // Номер порта
        struct in_addr sin_addr;    // IP-адрес
        char           sin_zero[8]; // Заполнитель для выравнивания
    };
     */
    sockaddr_in l_cl_addr = *(sockaddr_in *)l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons(l_port);
    freeaddrinfo(l_ai_ans);

    // socket creation
    int l_sock_server = socket(AF_INET, SOCK_STREAM, 0); // Возвращает дескриптор
    if (l_sock_server == -1)
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    // connect to server
    if (connect(l_sock_server, (sockaddr *)&l_cl_addr, sizeof(l_cl_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Unable to connect server.");
        exit(1);
    }

    uint l_lsa = sizeof(l_cl_addr);
    // my IP
    getsockname(l_sock_server, (sockaddr *)&l_cl_addr, &l_lsa);
    log_msg(LOG_INFO, "My IP: '%s'  port: %d",
            inet_ntoa(l_cl_addr.sin_addr), ntohs(l_cl_addr.sin_port));
    // server IP
    getpeername(l_sock_server, (sockaddr *)&l_cl_addr, &l_lsa);
    log_msg(LOG_INFO, "Server IP: '%s'  port: %d",
            inet_ntoa(l_cl_addr.sin_addr), ntohs(l_cl_addr.sin_port));

    log_msg(LOG_INFO, "Enter 'close' to close application.");

    // random number generator
    // srand(time(NULL));

    // list of fd sources
    /*
    struct pollfd {
        int   fd;       // Файловый дескриптор
        short events;   // Запрашиваемые события (что отслеживать)
        short revents;  // Возвращаемые события (что произошло)
    };
     */
    // pollfd l_read_poll[ 2 ];

    // STDIN_FILENO - файловый дескриптор ввода (клавиатура)
    // l_read_poll[ 0 ].fd = STDIN_FILENO;
    // l_read_poll[ 0 ].events = POLLIN;
    // l_read_poll[ 1 ].fd = l_sock_server;
    // l_read_poll[ 1 ].events = POLLIN;

    char l_buf[256];
    snprintf(l_buf, sizeof(l_buf), "%s\n", resolution);
    int l_len = write(l_sock_server, l_buf, strlen(l_buf));
    if (l_len < 0)
    {
        log_msg(LOG_ERROR, "Unable to send data to server!");
        close(l_sock_server);
        exit(1);
    }
    log_msg(LOG_DEBUG, "Sent %d bytes to server: %s", l_len, l_buf);

    FILE *fp = fopen("image.png", "wb");
    if (!fp)
    {
        log_msg(LOG_ERROR, "Unable to open file");
        close(l_sock_server);
        exit(1);
    }

    while ((l_len = read(l_sock_server, l_buf, sizeof(l_buf))) > 0)
    {
        fwrite(l_buf, 1, l_len, fp);
        log_msg(LOG_DEBUG, "Received %d bytes", l_len);
    }

    if (l_len < 0)
    {
        log_msg(LOG_ERROR, "Unable to read data from server!");
        fclose(fp);
        close(l_sock_server);
        exit(1);
    }

    log_msg(LOG_DEBUG, "Server closed socket.");
    fclose(fp);
    close(l_sock_server);

    pid_t display_pid = fork();
    if (display_pid < 0)
    {
        log_msg(LOG_ERROR, "Fork for display failed!");
        exit(1);
    }

    if (display_pid == 0)
    {
        execlp("open", "open", "image.png", NULL);
        log_msg(LOG_ERROR, "Exec display failed!");
        exit(1);
    }
    waitpid(display_pid, NULL, 0);
    return 0;

    /*
    // go!
    while ( 1 )
    {

        // select from fds
        // > 0: Количество дескрипторов, на которых произошли события (записаны в revents структур pollfd).
        // 0: Истёк тайм-аут (событий не произошло).
        // < 0: Произошла ошибка, код ошибки записывается в errno.
        int l_poll = poll( l_read_poll, 2, 5000 );

        if (l_poll < 0) break;

        if (l_poll == 0) {
            int num1 = rand() % 10 + 1;
            int num2 = rand() % 10 + 1;
            char op = rand() % 2 ? '+' : '-';
            char auto_buf[20];
            sprintf(auto_buf, "%d%c%d\n", num1, op, num2);
            int auto_len = strlen(auto_buf);

            auto_len = write(l_sock_server, auto_buf, auto_len);
            if (auto_len < 0) {
                log_msg(LOG_ERROR, "Unable to send auto data to server.");
                break;
            }

            log_msg(LOG_DEBUG, "Sent auto %d bytes to server: %s", auto_len, auto_buf);
            continue;
        }

        // data on stdin?
        if ( l_read_poll[ 0 ].revents & POLLIN )
        {
            //  read from stdin
            int l_len = read( STDIN_FILENO, l_buf, sizeof( l_buf ) );
            if ( l_len == 0 )
            {
                log_msg( LOG_DEBUG, "Stdin closed." );
                break;
            }

            if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to read from stdin." );
                break;
            }

            log_msg( LOG_DEBUG, "Read %d bytes from stdin.", l_len );

            // send data to server
            l_len = write( l_sock_server, l_buf, l_len );
            if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to send data to server." );
                break;
            }

            log_msg( LOG_DEBUG, "Sent %d bytes to server.", l_len );
        }

        // data from server?
        if ( l_read_poll[ 1 ].revents & POLLIN )
        {
            // read data from server
            int l_len = read( l_sock_server, l_buf, sizeof( l_buf ) );
            if ( l_len == 0 )
            {
                log_msg( LOG_DEBUG, "Server closed socket." );
                break;
            }

            if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to read data from server." );
                break;
            }

            log_msg( LOG_DEBUG, "Read %d bytes from server.", l_len );

            // display on stdout
            l_len = write( STDOUT_FILENO, l_buf, l_len );
            if ( l_len < 0 )
            {
                log_msg( LOG_ERROR, "Unable to write to stdout." );
                break;
            }

            // request to close?
            if ( !strncasecmp( l_buf, STR_CLOSE, strlen( STR_CLOSE ) ) )
            {
                log_msg( LOG_INFO, "Connection will be closed..." );
                break;
            }
        }
    }

    // close socket
    close( l_sock_server );
    return 0;
    */
}
