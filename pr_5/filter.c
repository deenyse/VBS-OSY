#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s even|odd\n", argv[0]);
        return 1;
    }
    char *mode = argv[1];
    int x;
    while (scanf("%d", &x) == 1)
    {
        if ((strcmp(mode, "even") == 0 && x % 2 == 0) ||
            (strcmp(mode, "odd") == 0 && x % 2 != 0))
        {
            printf("%d\n", x);
        }
    }
    return 0;
}
