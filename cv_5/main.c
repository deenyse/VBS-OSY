// calcpipe.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void maybe_verbose(int v, const char *fmt, ...)
{
    if (!v)
        return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[calcpipe] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

#define CHILDREN_COUNT 4

int main(int argc, char *argv[])
{
    int verbose = 0;
    int argi = 1;

    if (argc >= 2 && strcmp(argv[1], "-v") == 0)
    {
        verbose = 1;
        argi = 2;
    }

    if (argc - argi != 4)
    {
        fprintf(stderr, "Usage: %s [-v] K N filter_mode transform_mode\n", argv[0]);
        return EXIT_FAILURE;
    }

    int K = atoi(argv[argi]);
    char *N = argv[argi + 1];
    char *filter_mode = argv[argi + 2];
    char *transform_mode = argv[argi + 3];

    double sum = 0;

    int pipes[K][CHILDREN_COUNT][2];     
    pid_t pids[K][CHILDREN_COUNT];       

    for (int i = 0; i < K; i++)
    {
        for (int j = 0; j < CHILDREN_COUNT; j++)
        {
            if (pipe(pipes[i][j]) == -1)
                die("pipe");
        }

        // 0) ./gen
        maybe_verbose(verbose, "Spouštím ./gen %s", N);
        pid_t pid = fork();
        if (pid < 0) die("fork gen");
        if (pid == 0)
        {
            dup2(pipes[i][0][1], STDOUT_FILENO); // gen → p0 → stdout
            for (int j = 0; j < CHILDREN_COUNT; j++)
            {
                close(pipes[i][j][0]);
                close(pipes[i][j][1]);
            }
            char stri[11];
            sprintf(stri, "%d", i);
            char *args[] = {"./gen", N, stri, NULL};
            execvp(args[0], args);
            fprintf(stderr, "execvp failed for gen: %s\n", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        pids[i][0] = pid;
        close(pipes[i][0][1]);

        // 1) ./filter
        maybe_verbose(verbose, "Spouštím ./filter %s", filter_mode);
        pid = fork();
        if (pid < 0) die("fork filter");
        if (pid == 0)
        {
            dup2(pipes[i][0][0], STDIN_FILENO);   
            dup2(pipes[i][1][1], STDOUT_FILENO);  
            for (int j = 0; j < CHILDREN_COUNT; j++)
            {
                close(pipes[i][j][0]);
                close(pipes[i][j][1]);
            }
            char *args[] = {"./filter", filter_mode, NULL};
            execvp(args[0], args);
            fprintf(stderr, "execvp failed for filter: %s\n", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        pids[i][1] = pid;
        close(pipes[i][0][0]); 
        close(pipes[i][1][1]); 


        // 2) ./transform
        maybe_verbose(verbose, "Spouštím ./transform %s", transform_mode);
        pid = fork();
        if (pid < 0) die("fork transform");
        if (pid == 0)
        {
            dup2(pipes[i][1][0], STDIN_FILENO);
            dup2(pipes[i][2][1], STDOUT_FILENO);
            for (int j = 0; j < CHILDREN_COUNT; j++)
            {
                close(pipes[i][j][0]);
                close(pipes[i][j][1]);
            }
            char *args[] = {"./transform", transform_mode, NULL};
            execvp(args[0], args);
            fprintf(stderr, "execvp failed for transform: %s\n", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        pids[i][2] = pid;
        close(pipes[i][1][0]); 
        close(pipes[i][2][1]); 

        // 3) ./sum
        maybe_verbose(verbose, "Spouštím ./sum");
        pid = fork();
        if (pid < 0) die("fork sum");
        if (pid == 0)
        {
            dup2(pipes[i][2][0], STDIN_FILENO);
            dup2(pipes[i][3][1], STDOUT_FILENO);
            for (int j = 0; j < CHILDREN_COUNT; j++)
            {
                close(pipes[i][j][0]);
                close(pipes[i][j][1]);
            }
            char *args[] = {"./sum", NULL};
            execvp(args[0], args);
            fprintf(stderr, "execvp failed for sum: %s\n", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        pids[i][3] = pid;
        close(pipes[i][2][0]); 
        close(pipes[i][3][1]); 

    
        FILE *fp = fdopen(pipes[i][3][0], "r");
        if (!fp)
            die("fdopen");
        double x;
        while (fscanf(fp, "%lf", &x) == 1)
        {
            sum += x;
            fprintf(stderr, "[calcpipe] Výsledek pipeline #%d: %.3f\n", i+1,x);
        }
        fclose(fp); 
}

//wait for the children
    for (int i = 0; i < K; i++)
    {
        for (int j = 0; j < CHILDREN_COUNT; j++)
        {
            int status;
            pid_t w = waitpid(pids[i][j], &status, 0);
            if (w == -1)
            {
                perror("waitpid");
            }
            else
            {
                if (WIFEXITED(status))
                {
                    maybe_verbose(verbose, "Proces %d ukončen s kódem %d", (int)w, WEXITSTATUS(status));
                }
            }
        }
    }

    printf("[calcpipe] Průměrný výsledek: %.3f\n", sum / K);
    return EXIT_SUCCESS;
}
