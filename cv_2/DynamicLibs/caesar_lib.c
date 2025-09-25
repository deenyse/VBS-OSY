#include <string.h>

char *encrypt_line(char *line, int key)
{

    for (int i = 0; i < strlen(line); i++)
    {
        line[i] = line[i] + key;
    }

    return line;
}