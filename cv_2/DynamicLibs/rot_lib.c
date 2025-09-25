#include <string.h>

#include <string.h>

char *encrypt_line(char *line, int key)
{
    int len = strlen(line);

    key = (key % 26 + 26) % 26;

    for (int i = 0; i < len; i++)
    {
        if (line[i] >= 'a' && line[i] <= 'z')
        {
            line[i] = ((line[i] - 'a') + key) % 26 + 'a';
        }
        else if (line[i] >= 'A' && line[i] <= 'Z')
        {
            line[i] = ((line[i] - 'A') + key) % 26 + 'A';
        }
    }

    return line;
}
