#include <stdio.h>

int main(void)
{
    double x, sum = 0;
    while (scanf("%lf", &x) == 1)
        sum += x;
    printf("%.3f\n", sum);
    return 0;
}
