#include <string.h>

char *encrypt_line(char *line, int key)
{

    for (int i = 0; i < strlen(line); i++)
    {
        if ((line[i] >= 'a' && line[i] <= 'z'))
            line[i] = ((line[i] - 'a') + key) % 26 + 'a';

        if (line[i] >= 'A' && line[i] <= 'Z')
            line[i] = ((line[i] - 'A') + key) % 26 + 'A';
    }

    return line;
}