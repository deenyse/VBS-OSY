#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>

#define NUM_CHILDREN 3
#define BUFFER_SIZE 256 + 16

int main(void)
{
    int pipes[NUM_CHILDREN][2];
    pid_t pids[NUM_CHILDREN];

    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe error");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        printf("[rodič] Spouštím potomka %d\n", i + 1);

        pids[i] = fork();
        if (pids[i] == -1)
        {
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0)
        {
            if (i == 0)
            {                       // child 1
                close(pipes[0][1]); // close pipe 0 for write
                close(pipes[1][0]); // close pipe 1 for read
                close(pipes[2][0]); // close pipe 2 for read
                close(pipes[2][1]); // close pipe 2 for write
                char buf[BUFFER_SIZE - 16];

                int counter = 1;
                while (read(pipes[0][0], buf, BUFFER_SIZE - 16) > 0)
                {
                    printf("[potomek1] Přijal: \"%s\"\n", buf);

                    char c;
                    for (int j = 0; j < strlen(buf); j++)
                    {
                        c = buf[j];
                        if (j == 0 || buf[j - 1] == ' ')
                            buf[j] = tolower(buf[j]);
                    }

                    char result[BUFFER_SIZE];
                    sprintf(result, "%d. %s", counter++, buf);

                    printf("[potomek1] Posílám: \"%s\"\n", result);
                    write(pipes[1][1], result, strlen(result) + 1);
                }
                close(pipes[1][1]);

                exit(EXIT_SUCCESS);
            }
            else if (i == 1) // child 2
            {
                close(pipes[0][0]);
                close(pipes[0][1]);

                close(pipes[1][1]);

                close(pipes[2][0]);
                char buf[BUFFER_SIZE - 8];

                while (read(pipes[1][0], buf, BUFFER_SIZE - 8) > 0)
                {
                    int length = 0;

                    printf("[potomek2] Přijal: %s\n", buf);

                    char c;

                    for (int j = 0; j < strlen(buf); j++)
                    {
                        c = buf[j];
                        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                        {
                            length++;
                        }
                    }

                    char result[BUFFER_SIZE];
                    sprintf(result, "%s (%d)", buf, length);

                    printf("[potomek2] Posílám: %s \n", result);
                    write(pipes[2][1], result, strlen(result) + 1);
                }
                close(pipes[1][0]);
                close(pipes[2][1]);

                exit(EXIT_SUCCESS);
            }
            else if (i == 2) // child 3
            {
                close(pipes[0][0]);
                close(pipes[0][1]);

                close(pipes[1][1]);
                close(pipes[1][1]);

                close(pipes[2][1]);
                char buf[BUFFER_SIZE];

                while (read(pipes[2][0], buf, BUFFER_SIZE - 8) > 0)
                {
                    printf("[potomek 3] Přijal: \"%s\"\n", buf);

                    char c;
                    for (int j = 1; j < strlen(buf); j++)
                    {
                        c = buf[j];
                        if (buf[j - 1] == ' ')
                        {
                            buf[j] = toupper(buf[j]);
                        }
                    }

                    printf("[potomek3] Výstup: \"%s\" \n\n", buf);
                }

                close(pipes[2][0]);
                exit(EXIT_SUCCESS);
            }
        }
    }

    // --- Kód rodiče ---
    close(pipes[0][0]);
    close(pipes[1][0]);
    close(pipes[1][1]);
    close(pipes[2][0]);
    close(pipes[2][1]);

    FILE *fileStream = fopen("jmena.txt", "r");

    char name[BUFFER_SIZE - 16];
    while (fgets(name, BUFFER_SIZE - 16, fileStream))
    {
        name[strcspn(name, "\n")] = '\0';
        printf("[rodič] Čtu ze souboru: \"%s\"\n", name);

        write(pipes[0][1], name, strlen(name) + 1);

        sleep(1);
    }

    close(pipes[0][1]);
}