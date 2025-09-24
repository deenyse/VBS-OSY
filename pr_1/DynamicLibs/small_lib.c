#include <string.h>

int check_line(char *line)
{
    int counter = 0;

    for (int i = 0; i < strlen(line); i++)
    {
        if (line[i] >= 'a' && line[i] <= 'z')
            counter++;
    }

    return counter;
}