#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

void *encrypt_lib;
char *(*encrypt_line)();

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Not enough arguments were given\n");
        return EXIT_FAILURE;
    }
    int key = atoi(argv[1]);
    char *line = NULL;
    size_t len = 0;

    encrypt_lib = dlopen("libencrypt.so", RTLD_LAZY);
    if (!encrypt_lib)
    {
        fprintf(stderr, "Failed to load library");
        return EXIT_FAILURE;
    }

    encrypt_line = dlsym(encrypt_lib, "encrypt_line");
    if (!encrypt_line)
    {
        fprintf(stderr, "Failed to find symbol 'encrypt_line'");
        dlclose(encrypt_lib);
        return EXIT_FAILURE;
    }

    while (getline(&line, &len, stdin) != -1)
    {
        printf("%s", encrypt_line(line, key));
    }

    return 0;
}
