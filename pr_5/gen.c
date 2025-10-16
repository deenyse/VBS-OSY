#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s N\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    for (int i = 1; i <= N; i++)
        printf("%d\n", i);
    return 0;
}
