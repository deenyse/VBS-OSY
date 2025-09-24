#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "StaticLibs/libgenstr.h"
int main(int argc, char *argv[])
{
    // srand(time(NULL));

    int lines_amount = atoi(argv[1]);
    int words_amount = atoi(argv[2]);

    // 65-90; 97-122sdfds

    for (int i = 0; i < lines_amount; i++)
    {
        printf("%s\n", genLine(words_amount));
    }

    return 0;
}