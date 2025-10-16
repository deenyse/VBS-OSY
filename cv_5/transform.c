#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s square|sqrt|neg\n", argv[0]);
        return 1;
    }
    char *mode = argv[1];
    double x;
    while (scanf("%lf", &x) == 1)
    {
        if (strcmp(mode, "square") == 0)
            x = x * x;
        else if (strcmp(mode, "sqrt") == 0)
            x = sqrt(x);
        else if (strcmp(mode, "neg") == 0)
            x = -x;
        printf("%g\n", x);
    }
    return 0;
}
