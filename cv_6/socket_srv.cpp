//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// This program is an example of a socket server. It allows connecting and
// serving multiple clients simultaneously using the fork() system call.
// The mandatory argument of the program is the port number for listening.
//
//***************************************************************************

#include <stdio.h>      // Standard I/O functions (printf, fprintf)
#include <stdlib.h>     // General utilities (exit, atoi, srand)
#include <unistd.h>     // POSIX operating system API (fork, pipe, read, write, close, execlp)
#include <string.h>     // String manipulation functions (strcmp, strlen, strncmp, strerror)
#include <fcntl.h>      // File control options
#include <stdarg.h>     // Handling variable argument lists
#include <poll.h>       // I/O multiplexing
#include <sys/socket.h> // Core socket functions and data structures
#include <sys/param.h>  // System parameters
#include <sys/time.h>   // Time-related functions
#include <sys/types.h>  // Basic system data types
#include <netinet/in.h> // Internet address family structures (sockaddr_in)
#include <arpa/inet.h>  // Functions for manipulating IP addresses (inet_ntoa)
#include <errno.h>      // System error numbers
#include <sys/wait.h>   // For waitpid() to manage child processes
#include <vector>       // C++ vector container
#include <string>       // C++ string class

// Predefined strings for commands
#define STR_CLOSE "close"
#define STR_QUIT "quit"

//***************************************************************************
// log messages

#define LOG_ERROR 0 // Log level for errors
#define LOG_INFO 1  // Log level for information and notifications
#define LOG_DEBUG 2 // Log level for debug messages

// Global debug flag, controls the verbosity of logging
int g_debug = LOG_INFO;

// Function for formatted logging to stdout or stderr
void log_msg(int t_log_level, const char *t_form, ...)
{
    // Format strings for different log levels
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n", // Error: includes errno and its string representation
        "INF: %s\n",         // Info
        "DEB: %s\n"          // Debug
    };

    // Skip logging if the message's log level is higher than the global debug level
    if (t_log_level > g_debug)
        return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);        // Initialize variable arguments list
    vsprintf(l_buf, t_form, l_arg); // Format the string into a buffer
    va_end(l_arg);                  // Clean up the variable arguments list

    switch (t_log_level)
    {
    case LOG_INFO:
    case LOG_DEBUG:
        // Print info and debug messages to standard output
        fprintf(stdout, out_fmt[t_log_level], l_buf);
        break;

    case LOG_ERROR:
        // Print error messages to standard error, including system error info
        fprintf(stderr, out_fmt[t_log_level], errno, strerror(errno), l_buf);
        break;
    }
}

//***************************************************************************
// help

// Displays help information and exits the program.
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

    // Enable debug mode if -d is specified
    if (!strcmp(t_args[1], "-d"))
        g_debug = LOG_DEBUG;
}

//***************************************************************************
// Function to process a file by sha1sum
void process_batch(int t_sock_client, char* expression)
{
    log_msg(LOG_INFO, "Processing a file %s.", expression);
    
    // write(t_sock_client, expression.c_str(), expression.length());

    // // Execute python3. The process image is replaced.
    // execlp("python3", "python3", NULL);
    // // This code is only reached if execlp fails
    // log_msg(LOG_ERROR, "Exec python3 failed!");

    // Create a pipe for inter-process communication
    int pipe_fd[2]; // pipe_fd[0] is for reading, pipe_fd[1] is for writing
    if (pipe(pipe_fd) == -1)
    {
        log_msg(LOG_ERROR, "Unable to create a pipe.");
        return;
    }

    // Fork a grandchild process to execute Python
    pid_t py_pid = fork();
    if (py_pid < 0)
    {
        log_msg(LOG_ERROR, "Fork for Python failed!");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    }
    

    if (py_pid == 0)
    {
        // Grandchild process: executes sha1sum
        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin to read from the pipe
        close(pipe_fd[0]);              // Close the original pipe read descriptor

        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to the pipe
        close(t_sock_client);               // Close the original socket descriptor


        // Execute sha1sum. The process image is replaced.
        char *args[] = {"sha1sum", expression, NULL};
        execvp(args[0], args);
        
        // This code is only reached if execlp fails
        log_msg(LOG_ERROR, "Exec sha1sum failed!");
        exit(1);
    }
    close(pipe_fd[1]); // Close the write-end, signaling EOF to the reader
    char buf[256];

    read(pipe_fd[0], buf, 256);
    
    close(pipe_fd[0]); // Close the read-end of the pipe

    // Wait for the grandchild to finish to avoid creating a zombie process
    waitpid(py_pid, NULL, 0);

    //get only hash from sha1sum
    char *space_pos = strchr(buf, ' ');
    if (space_pos != NULL) {
        *space_pos = '\0'; 
    }
    
    std::string out_string;
    //write hash into out_string

    out_string += buf;
    // sprintf(out_string, "%s", buf);
    
    //GET FILE TYPE
    //
    //
    //
    if (pipe(pipe_fd) == -1)
    {
        log_msg(LOG_ERROR, "Unable to create a pipe.");
        return;
    }

    // Fork a grandchild process to execute Python
    py_pid = fork();
    if (py_pid < 0)
    {
        log_msg(LOG_ERROR, "Fork for Python failed!");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    }
    

    if (py_pid == 0)
    {
        // Grandchild process: executes sha1sum
        dup2(pipe_fd[0], STDIN_FILENO); // Redirect stdin to read from the pipe
        close(pipe_fd[0]);              // Close the original pipe read descriptor

        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to the pipe
        close(t_sock_client);               // Close the original socket descriptor


        // Execute sha1sum. The process image is replaced.
        char *args[] = {"file", expression, NULL};
        execvp(args[0], args);
        
        // This code is only reached if execlp fails
        log_msg(LOG_ERROR, "Exec file failed!");
        exit(1);
    }
    close(pipe_fd[1]); // Close the write-end, signaling EOF to the reader
    

    read(pipe_fd[0], buf, 256);
    
    close(pipe_fd[0]); // Close the read-end of the pipe
    waitpid(py_pid, NULL, 0);


    space_pos = strchr(buf, '\n');
    if (space_pos != NULL) {
        *space_pos = '\0'; 
    }
    
    //get only filetype
    space_pos = strchr(buf, ' ');

    out_string += space_pos;
    out_string += "\0";
    //write hash into out_string
    // sprintf(out_string, "%s", space_pos);

    log_msg(LOG_INFO, "Sending out: %s", out_string.c_str());
    write(t_sock_client, out_string.c_str(), out_string.length());

}

//***************************************************************************
// Function to handle communication with a single client (runs in a child process)
void handle_client(int t_sock_client)
{
    log_msg(LOG_INFO, "Child process handling client on socket %d.", t_sock_client);

    // Poll structure to monitor the client socket for incoming data
    pollfd l_read_poll[1];
    l_read_poll[0].fd = t_sock_client;
    l_read_poll[0].events = POLLIN; // Monitor for readability (incoming data)


    // Main loop for handling a client
    while (1)
    {
        char l_buf[256] = {0};

        // Wait for data from the client indefinitely (-1 timeout)
        int l_poll = poll(l_read_poll, 1, -1);

        if (l_poll < 0)
        {
            log_msg(LOG_ERROR, "Poll function failed in child process!");
            break;
        }

        // Check if data is available on the client socket
        if (l_read_poll[0].revents & POLLIN)
        {
            // Read data from the socket
            int l_len = read(t_sock_client, l_buf, sizeof(l_buf) - 1);
            if (l_len <= 0)
            {
                if (l_len == 0)
                    log_msg(LOG_DEBUG, "Client closed the socket!");
                else
                    log_msg(LOG_ERROR, "Unable to read data from client.");
                break; // Exit loop on error or connection close
            }

            l_buf[l_len] = '\0'; // Null-terminate the received string
            log_msg(LOG_DEBUG, "Read %d bytes from client: %s", l_len, l_buf);

            // Remove the newline character, if present
            char *newline = strchr(l_buf, '\n');
            if (newline)
                *newline = '\0';

            // Check for a "close" command to terminate the connection
            if (!strncasecmp(l_buf, STR_CLOSE, strlen(STR_CLOSE)))
            {
                log_msg(LOG_INFO, "Client sent 'close' request.");
                break;
            }


            process_batch(t_sock_client, l_buf);
            
        }
    }

    // Clean up before the child process exits
    close(t_sock_client);
    log_msg(LOG_INFO, "Connection closed.");
    exit(0); // Terminate the child process
}

//***************************************************************************
// Main function
int main(int t_narg, char **t_args)
{
    if (t_narg <= 1)
        help(t_narg, t_args);

    int l_port = 0;

    // Parse command-line arguments
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
        exit(1);
    }

    log_msg(LOG_INFO, "Server will listen on port: %d.", l_port);

    // Create a socket for listening
    int l_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_listen == -1)
    {
        log_msg(LOG_ERROR, "Unable to create a socket.");
        exit(1);
    }

    // Define the server address structure
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;         // IPv4
    l_srv_addr.sin_port = htons(l_port);     // Port number in network byte order
    l_srv_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces

    // Set socket option to allow reusing the address, useful for quick restarts
    int l_opt = 1;
    if (setsockopt(l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0)
        log_msg(LOG_ERROR, "Unable to set socket option!");

    // Bind the socket to the specified address and port
    if (bind(l_sock_listen, (const sockaddr *)&l_srv_addr, sizeof(l_srv_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Bind failed!");
        close(l_sock_listen);
        exit(1);
    }

    // Put the socket into listening state, with a backlog of 5 pending connections
    if (listen(l_sock_listen, 5) < 0)
    {
        log_msg(LOG_ERROR, "Unable to listen on the given port!");
        close(l_sock_listen);
        exit(1);
    }

    log_msg(LOG_INFO, "Enter 'quit' to stop the server.");

    // Main server loop (parent process)
    while (1)
    {
        // Poll structure to monitor stdin and the listening socket
        pollfd l_read_poll[2];
        l_read_poll[0].fd = STDIN_FILENO; // Monitor stdin for the 'quit' command
        l_read_poll[0].events = POLLIN;
        l_read_poll[1].fd = l_sock_listen; // Monitor the listening socket for new connections
        l_read_poll[1].events = POLLIN;

        poll(l_read_poll, 2, -1);

        // Check for input on stdin (server quit command)
        if (l_read_poll[0].revents & POLLIN)
        {
            char buf[128];
            int l_len = read(STDIN_FILENO, buf, sizeof(buf));
            if (l_len > 0 && !strncmp(buf, STR_QUIT, strlen(STR_QUIT)))
            {
                log_msg(LOG_INFO, "Request to 'quit' entered. Shutting down.");
                break; // Exit the main loop
            }
        }

        // Check for a new client connection attempt
        if (l_read_poll[1].revents & POLLIN)
        {
            sockaddr_in l_rsa; // To store client address information
            socklen_t l_rsa_size = sizeof(l_rsa);
            // Accept the new connection, creating a new socket for communication
            int l_sock_client = accept(l_sock_listen, (sockaddr *)&l_rsa, &l_rsa_size);
            if (l_sock_client == -1)
            {
                log_msg(LOG_ERROR, "Unable to accept new client.");
                continue;
            }

            log_msg(LOG_INFO, "New client connected from IP: '%s' port: %d",
                    inet_ntoa(l_rsa.sin_addr), ntohs(l_rsa.sin_port));

            // Fork a child process to handle the new client
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
                close(l_sock_listen);         // The child does not need the listening socket
                handle_client(l_sock_client); // Handle the client communication
                // The child process will exit inside handle_client
            }
            else
            {
                // Parent process
                close(l_sock_client); // The parent does not need the client communication socket
                log_msg(LOG_INFO, "Forked child PID %d to handle the client.", l_pid);
                // Periodically clean up terminated child (zombie) processes without blocking
                waitpid(-1, NULL, WNOHANG);
            }
        }
    }

    // Close the listening socket before the server terminates
    close(l_sock_listen);
    return 0;
}