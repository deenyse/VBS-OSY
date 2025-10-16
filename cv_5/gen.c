#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s N PID\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    int PID = atoi(argv[2]);
    srand(PID);
    for (int i = 1; i <= N; i++)
        printf("%d\n", rand()%10);
    return 0;
}
