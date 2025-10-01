#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
void printSize(char *fileName)
{
    struct stat sb;

    stat(fileName, &sb);

    printf("File size:%jd bytes\n", (intmax_t)sb.st_size);

    // Print time by asctime. Why just not to use casual ctime?
    // TODO: Put it into separate function
    printf("Last file modification:   %s", asctime(gmtime(&sb.st_mtime)));
}

int main(int argc, char *argv[])
{

    bool s = false;
    bool t = false;
    bool n = false;
    char *fileLocation = NULL;

    // TODO: Need to fix output according to arrangement
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp("-s", argv[i]))
            s = true;
        else if (!strcmp("-t", argv[i]))
            t = true;
        else if (!strcmp("-n", argv[i]))
            n = true;
        else
            fileLocation = argv[i];
    }

    if (fileLocation == NULL)
    {
        fprintf(stderr, "No file location is provided");
        return 1;
    }

    printSize(fileLocation);
    printf("-s %d; -t %d; -n %d; Location %s;", s, t, n, fileLocation);

    return 0;
}