#include <string.h>

char *encrypt_line(char *line, int key)
{

    for (int i = 0; i < strlen(line); i++)
    {
        if (line[i] != '\n')
            line[i] = line[i] ^ key;
    }

    return line;
}