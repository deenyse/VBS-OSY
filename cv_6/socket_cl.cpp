//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of a socket client.
// The mandatory arguments of the program are the IP address or name of the
// server and a port number.
//
//***************************************************************************

#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // General utilities (exit, atoi, srand, rand)
#include <unistd.h>     // POSIX operating system API (read, write, close)
#include <string.h>     // String manipulation functions
#include <fcntl.h>      // File control options
#include <stdarg.h>     // Handling variable argument lists
#include <poll.h>       // I/O multiplexing
#include <sys/socket.h> // Core socket functions and data structures
#include <sys/param.h>  // System parameters
#include <sys/time.h>   // Time-related functions
#include <sys/types.h>  // Basic system data types
#include <netinet/in.h> // Internet address family structures (sockaddr_in)
#include <arpa/inet.h>  // Functions for manipulating IP addresses
#include <errno.h>      // System error numbers
#include <netdb.h>      // For getaddrinfo() to resolve hostnames
#include <time.h>       // For time() to seed the random number generator
#include <string>
#define STR_CLOSE "close"

//***************************************************************************
// log messages

#define LOG_ERROR 0 // Log level for errors
#define LOG_INFO 1  // Log level for information and notifications
#define LOG_DEBUG 2 // Log level for debug messages

// Global debug flag
int g_debug = LOG_INFO;

// Function for formatted logging
void log_msg(int t_log_level, const char *t_form, ...)
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"};

    if (t_log_level > g_debug)
        return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsnprintf(l_buf, sizeof(l_buf), t_form, l_arg);
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

// Displays help information and exits.
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
// Main function
int main(int t_narg, char **t_args)
{
    if (t_narg <= 2)
        help(t_narg, t_args);

    int l_port = 0;
    char *l_host = nullptr;

    int l_moove = 0;
    // Parse command-line arguments
    for (int i = 1; i < 3; i++)
    {
        if (!strcmp(t_args[i], "-d")){
            g_debug = LOG_DEBUG;
            l_moove ++;
        }
        if (!strcmp(t_args[i], "-h"))
        {
            help(t_narg, t_args);
            l_moove++;
        }
    }

    l_host = t_args[l_moove + 1];
    l_port = atoi(t_args[l_moove +2]);


    if (!l_host || !l_port)
    {
        log_msg(LOG_INFO, "Host or port is missing!");
        help(t_narg, t_args);
        exit(1);
    }

    log_msg(LOG_INFO, "Attempting to connect to '%s' on port %d.", l_host, l_port);

    // Resolve the server's address
    addrinfo l_ai_req, *l_ai_ans;
    bzero(&l_ai_req, sizeof(l_ai_req));
    l_ai_req.ai_family = AF_INET;       // Use IPv4
    l_ai_req.ai_socktype = SOCK_STREAM; // Use TCP

    // getaddrinfo resolves hostname to an address, results are in l_ai_ans
    if (getaddrinfo(l_host, nullptr, &l_ai_req, &l_ai_ans) != 0)
    {
        log_msg(LOG_ERROR, "Unknown host name!");
        exit(1);
    }

    // Copy the resolved address information
    sockaddr_in l_cl_addr = *(sockaddr_in *)l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons(l_port); // Set the port in network byte order
    freeaddrinfo(l_ai_ans);             // Free the memory allocated by getaddrinfo

    // Create a socket
    int l_sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_server == -1)
    {
        log_msg(LOG_ERROR, "Unable to create a socket.");
        exit(1);
    }

    // Connect to the server
    if (connect(l_sock_server, (sockaddr *)&l_cl_addr, sizeof(l_cl_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Unable to connect to the server.");
        exit(1);
    }

    log_msg(LOG_INFO, "Successfully connected.");


    // Array of pollfd structures to monitor multiple file descriptors
    pollfd l_read_poll[2];
    l_read_poll[0].fd = STDIN_FILENO;  // Monitor standard input (keyboard)
    l_read_poll[0].events = POLLIN;    // Check for incoming data
    l_read_poll[1].fd = l_sock_server; // Monitor the server socket
    l_read_poll[1].events = POLLIN;    // Check for incoming data


    //SEND ALL FILENAMES
    //
    //
    //
    //

        
    int i =l_moove + 3;
    // Main client loop
    while (1)
    {
        char l_buf[256];
        
        // Wait for an event on stdin or the socket, with a 5-second timeout (5000 ms)

        int l_poll = poll(l_read_poll, 2, 200);

        if (l_poll < 0)
        {
            log_msg(LOG_ERROR, "Poll failed!");
            break;
        }

        // file_d = fopen(...)

        // while (len = read(file_d, buf, sizeof(buf))) {
        //      write(socket_se)
        // }

        if (l_poll == 0)
        {
            if(i < t_narg){
                write(l_sock_server, t_args[i], strlen(t_args[i]));
                i++;
            }
            continue; // Go back to waiting
        }

        // Data is available on standard input (user typed something)
        if (l_read_poll[0].revents & POLLIN)
        {
            int l_len = read(STDIN_FILENO, l_buf, sizeof(l_buf));
            if (l_len <= 0)
            {
                log_msg(LOG_DEBUG, "Stdin closed.");
                break;
            }

            // Check if the user wants to close the connection
            if (!strncasecmp(l_buf, STR_CLOSE, strlen(STR_CLOSE)))
            {
                log_msg(LOG_INFO, "Connection will be closed...");
                break;
            }
        }

        // Data is available from the server
        if (l_read_poll[1].revents & POLLIN)
        {
            int l_len = read(l_sock_server, l_buf, sizeof(l_buf) - 1);
            if (l_len <= 0)
            {
                log_msg(LOG_INFO, "Server closed the connection.");
                break;
            }
            l_buf[l_len] = '\0'; // Null-terminate the received data

            // Display the server's response
            log_msg(LOG_INFO, "Received from server:\n%s", l_buf);
        }
    }

    // Close the socket before exiting
    close(l_sock_server);
    return 0;
}