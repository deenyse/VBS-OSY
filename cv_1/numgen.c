#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
int main(int argc, char *argv[])
{

    int MIN;
    int MAX;
    int COUNT;
    char *fileLocation = NULL;
    bool isBinary = false;

    isBinary = !strcmp(argv[1], "-b");

    if (argc == 4)
    {
        isBinary = false;
        MIN = atoi(argv[1]);
        MAX = atoi(argv[2]);
        COUNT = atoi(argv[3]);
    }
    else if (argc == 5 && !isBinary)
    {
        MIN = atoi(argv[1]);
        MAX = atoi(argv[2]);
        COUNT = atoi(argv[3]);
        fileLocation = argv[4];
    }
    else if (argc == 5 && isBinary)
    {
        MIN = atoi(argv[2]);
        MAX = atoi(argv[3]);
        COUNT = atoi(argv[4]);
        fileLocation = argv[5];
    }

    int nums[COUNT];

    for (int i = 0; i < COUNT; i++)
    {
        nums[i] = rand() % (MAX - MIN);
    }

    if (fileLocation != NULL)
    {
        FILE *file = fopen(fileLocation, "w+");
        if (file == NULL)
        {
            fprintf(stderr, "Error: could not open output file '%s'\n", fileLocation);
        }
        for (int i = 0; i < COUNT; i++)
        {
            if (isBinary)
                fwrite(&nums[i], sizeof(int), 1, file);
            else
                fprintf(file, "%d ", nums[i]);
        }
    }
    else
    {
        for (int i = 0; i < COUNT; i++)
        {
            printf("%d ", nums[i]);
        }
    }

    return 0;
}
