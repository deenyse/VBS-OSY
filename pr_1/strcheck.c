#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

// int check_line(char *line);

void *counter_lib;
int (*check_line)();

int main(int argc, char *argv[])
{
    int counter = 0;
    char *line = NULL;
    size_t len = 0;

    counter_lib = dlopen("libcounter.so", RTLD_LAZY);
    check_line = dlsym(counter_lib, "check_line");

    while (getline(&line, &len, stdin) != -1)
    {
        counter += check_line(line);
    }

    printf("%d", counter);

    return 0;
}