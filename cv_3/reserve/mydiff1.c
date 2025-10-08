#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>

typedef struct
{
    char *name;
    FILE *fp;
    off_t offset;
    off_t size;
    mode_t mode;
} FileInfo;

void mode_to_str(mode_t m, char *s)
{
    char rwx[3] = {'r', 'w', 'x'};
    for (int i = 0; i < 9; i++)
    {
        s[i] = (m & (1 << (8 - i))) ? rwx[i % 3] : '-';
    }
    s[9] = '\0';
}

void format_time(time_t t, char *buf, size_t n)
{
    strftime(buf, n, "%Y-%m-%d %H:%M:%S", localtime(&t));
}

void compare(FILE *a, char *name1, FILE *b, char *name2)
{
    char *l1 = NULL, *l2 = NULL;
    size_t n1 = 0, n2 = 0;
    ssize_t r1, r2;
    rewind(a);
    rewind(b);
    while (1)
    {
        r1 = getline(&l1, &n1, a);
        r2 = getline(&l2, &n2, b);
        if (r1 == -1 && r2 == -1)
            break;

        if (strcmp(l1, l2) != 0)
        {
            printf("%s: %s", name1, l1);
            // if (l1[strlen(l1) - 1] != '\n')
            //     printf("\n");
            printf("%s: %s", name2, l2);
            // if (l2[strlen(l2) - 1] != '\n')
            //     printf("\n");
        }
    }
    free(l1);
    free(l2);
}

void read_new(FileInfo *f)
{
    fseeko(f->fp, f->offset, SEEK_SET);
    char buf[1024];
    while (fgets(buf, sizeof(buf), f->fp))
    {
        printf("%s: %s", f->name, buf);
        if (buf[strlen(buf) - 1] != '\n')
            printf("\n");
    }
    f->offset = ftello(f->fp);
}

int main(int argc, char *argv[])
{
    int show_size = 0, show_time = 0;
    int opt;
    int optind = 0;
    while ((opt = getopt(argc, argv, "st")) != -1)
    {
        if (opt == 's')
        {
            show_size = 1;
            optind++;
        }
        if (opt == 't')
        {
            show_time = 1;
            optind++;
        }
    }

    FileInfo f1, f2;
    f1.name = argv[optind];
    f2.name = argv[optind + 1];
    f1.fp = fopen(f1.name, "r");
    f2.fp = fopen(f2.name, "r");
    if (!f1.fp || !f2.fp)
    {
        perror("fopen");
        return 1;
    }

    struct stat st1, st2;
    stat(f1.name, &st1);
    stat(f2.name, &st2);
    f1.size = st1.st_size;
    f2.size = st2.st_size;
    f1.mode = st1.st_mode & 0777;
    f2.mode = st2.st_mode & 0777;
    f1.offset = f1.size;
    f2.offset = f2.size;

    char time_buf1[64], time_buf2[64];
    char m1[10], m2[10];
    if (show_size || show_time)
    {
        mode_to_str(f1.mode, m1);
        mode_to_str(f2.mode, m2);
        if (show_time)
        {
            format_time(st1.st_mtime, time_buf1, sizeof(time_buf1));
            format_time(st2.st_mtime, time_buf2, sizeof(time_buf2));
        }

        printf("%s (", f1.name);
        if (show_size)
            printf("SIZE=%lld", (long long)f1.size);
        if (show_size && show_time)
            printf(", ");
        if (show_time)
            printf("MTIME=%s", time_buf1);
        printf(", MODE=%s)\n", m1);

        printf("%s (", f2.name);
        if (show_size)
            printf("SIZE=%lld", (long long)f2.size);
        if (show_size && show_time)
            printf(", ");
        if (show_time)
            printf("MTIME=%s", time_buf2);
        printf(", MODE=%s)\n", m2);
    }

    compare(f1.fp, f1.name, f2.fp, f2.name);

    while (1)
    {
        sleep(1);
        if (stat(f1.name, &st1) == -1)
        {
            format_time(time(NULL), time_buf1, sizeof(time_buf1));
            printf("[%s] %s zmizel\n", time_buf1, f1.name);
            break;
        }
        if (stat(f2.name, &st2) == -1)
        {
            format_time(time(NULL), time_buf2, sizeof(time_buf2));
            printf("[%s] %s zmizel\n", time_buf2, f2.name);
            break;
        }

        if (st1.st_size > f1.size)
            // read_new(&f1);
            if (st2.st_size > f2.size)
                // read_new(&f2);

                if (st1.st_size < f1.size)
                {
                    format_time(time(NULL), time_buf1, sizeof(time_buf1));
                    printf("[%s] %s zkrácen z %lld na %lld bajtů\n", time_buf1, f1.name, (long long)f1.size, (long long)st1.st_size);
                    fseeko(f1.fp, 0, SEEK_END);
                    f1.offset = st1.st_size;
                }
        if (st2.st_size < f2.size)
        {
            format_time(time(NULL), time_buf2, sizeof(time_buf2));
            printf("[%s] %s zkrácen z %lld na %lld bajtů\n", time_buf2, f2.name, (long long)f2.size, (long long)st2.st_size);
            fseeko(f2.fp, 0, SEEK_END);
            f2.offset = st2.st_size;
        }
        f1.size = st1.st_size;
        f2.size = st2.st_size;

        mode_t new1 = st1.st_mode & 0777, new2 = st2.st_mode & 0777;
        if (new1 != f1.mode)
        {
            char old[10], nw[10];
            mode_to_str(f1.mode, old);
            mode_to_str(new1, nw);
            format_time(time(NULL), time_buf1, sizeof(time_buf1));
            printf("[%s] %s změna práv: %s -> %s\n", time_buf1, f1.name, old, nw);
            f1.mode = new1;
        }
        if (new2 != f2.mode)
        {
            char old[10], nw[10];
            mode_to_str(f2.mode, old);
            mode_to_str(new2, nw);
            format_time(time(NULL), time_buf2, sizeof(time_buf2));
            printf("[%s] %s změna práv: %s -> %s\n", time_buf2, f2.name, old, nw);
            f2.mode = new2;
        }
    }
    fclose(f1.fp);
    fclose(f2.fp);
    return 0;
}
