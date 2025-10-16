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

int main(int argc, char *argv[])
{
    int verbose = 0;
    int argi = 1;

    if (argc >= 2 && strcmp(argv[1], "-v") == 0)
    {
        verbose = 1;
        argi = 2;
    }

    if (argc - argi != 3)
    {
        fprintf(stderr, "Usage: %s [-v] N filter_mode transform_mode\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *N = argv[argi];
    char *filter_mode = argv[argi + 1];
    char *transform_mode = argv[argi + 2];

    /* vytvoříme 3 roury: p1 mezi gen->filter, p2 mezi filter->transform, p3 mezi transform->sum */
    int p1[2], p2[2], p3[2];
    if (pipe(p1) == -1)
        die("pipe p1");
    if (pipe(p2) == -1)
        die("pipe p2");
    if (pipe(p3) == -1)
        die("pipe p3");

    pid_t pids[4];

    /* 1) gen */
    maybe_verbose(verbose, "Spouštím ./gen %s", N);
    pid_t pid = fork();
    if (pid < 0)
        die("fork gen");
    if (pid == 0)
    {
        /* child gen: stdout -> p1[1] */
        if (dup2(p1[1], STDOUT_FILENO) == -1)
            die("dup2 gen stdout");
        /* zavřít všechny nepotřebné fds */
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        close(p3[0]);
        close(p3[1]);

        char *args[] = {"./gen", N, NULL};
        execvp(args[0], args);
        /* pokud exec selže */
        fprintf(stderr, "execvp failed for gen: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    pids[0] = pid;
    /* rodič zavře write konec p1 (protože ho používá child gen) */
    close(p1[1]);

    /* 2) filter */
    maybe_verbose(verbose, "Spouštím ./filter %s", filter_mode);
    pid = fork();
    if (pid < 0)
        die("fork filter");
    if (pid == 0)
    {
        /* stdin <- p1[0], stdout -> p2[1] */
        if (dup2(p1[0], STDIN_FILENO) == -1)
            die("dup2 filter stdin");
        if (dup2(p2[1], STDOUT_FILENO) == -1)
            die("dup2 filter stdout");
        /* zavřít všechno */
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        close(p3[0]);
        close(p3[1]);

        char *args[] = {"./filter", filter_mode, NULL};
        execvp(args[0], args);
        fprintf(stderr, "execvp failed for filter: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    pids[1] = pid;
    /* rodič zavře read konec p1 a write konec p2 */
    close(p1[0]);
    close(p2[1]);

    /* 3) transform */
    maybe_verbose(verbose, "Spouštím ./transform %s", transform_mode);
    pid = fork();
    if (pid < 0)
        die("fork transform");
    if (pid == 0)
    {
        /* stdin <- p2[0], stdout -> p3[1] */
        if (dup2(p2[0], STDIN_FILENO) == -1)
            die("dup2 transform stdin");
        if (dup2(p3[1], STDOUT_FILENO) == -1)
            die("dup2 transform stdout");
        /* zavřít všechno */
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        close(p3[0]);
        close(p3[1]);

        char *args[] = {"./transform", transform_mode, NULL};
        execvp(args[0], args);
        fprintf(stderr, "execvp failed for transform: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    pids[2] = pid;
    /* rodič zavře read konec p2 a write konec p3 */
    close(p2[0]);
    close(p3[1]);

    /* 4) sum */
    maybe_verbose(verbose, "Spouštím ./sum");
    pid = fork();
    if (pid < 0)
        die("fork sum");
    if (pid == 0)
    {
        /* stdin <- p3[0] */
        if (dup2(p3[0], STDIN_FILENO) == -1)
            die("dup2 sum stdin");
        /* zavřít všechno */
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        close(p3[0]);
        close(p3[1]);

        char *args[] = {"./sum", NULL};
        execvp(args[0], args);
        fprintf(stderr, "execvp failed for sum: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    pids[3] = pid;
    /* rodič zavře read konec p3 */
    close(p3[0]);

    /* rodič čeká na všechny children */
    int status;
    for (int i = 0; i < 4; ++i)
    {
        pid_t w = waitpid(pids[i], &status, 0);
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
            else if (WIFSIGNALED(status))
            {
                maybe_verbose(verbose, "Proces %d ukončen signálem %d", (int)w, WTERMSIG(status));
            }
        }
    }

    return EXIT_SUCCESS;
}
