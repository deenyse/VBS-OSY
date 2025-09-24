#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [-b] operation [input_file output_file]\n", argv[0]);
        return 1;
    }

    bool isBinary = strcmp(argv[1], "-b") == 0 ? true : false;

    char *operation = argv[1 + isBinary];

    FILE *input = stdin;
    FILE *output = stdout;

    if (argc - 1 - isBinary == 3)
    {
        input = fopen(argv[2 + isBinary], "r");
        if (!input)
        {
            fprintf(stderr, "Error: could not open input file '%s'\n", argv[2 + isBinary]);
            return 1;
        }
        output = fopen(argv[3 + isBinary], isBinary ? "wb" : "w");
        if (!output)
        {
            fprintf(stderr, "Error: could not open output file '%s'\n", argv[3 + isBinary]);
            fclose(input);
            return 1;
        }
    }

    long long sum = 0;
    int count = 0;
    int value;
    int min = INT_MAX, max = INT_MIN;

    while (fscanf(input, "%d", &value) == 1)
    {
        sum += value;
        if (value < min)
            min = value;
        if (value > max)
            max = value;
        count++;
    }

    if (count == 0)
    {
        fprintf(stderr, "Error: no numbers in input\n");
        if (input != stdin)
            fclose(input);
        if (output != stdout)
            fclose(output);
        return 1;
    }

    if (strcmp(operation, "sum") == 0)
    {
        if (isBinary)
            fwrite(&sum, sizeof(long long), 1, output);
        else
            fprintf(output, "%lld\n", sum);
    }
    else if (strcmp(operation, "avg") == 0)
    {
        double avg = (double)sum / count;
        if (isBinary)
            fwrite(&avg, sizeof(double), 1, output);
        else
            fprintf(output, "%.2f\n", avg);
    }
    else if (strcmp(operation, "min") == 0)
    {
        if (isBinary)
            fwrite(&min, sizeof(int), 1, output);
        else
            fprintf(output, "%d\n", min);
    }
    else if (strcmp(operation, "max") == 0)
    {
        if (isBinary)
            fwrite(&max, sizeof(int), 1, output);
        else
            fprintf(output, "%d\n", max);
    }
    else
    {
        fprintf(stderr, "Error: unknown operation '%s'\n", operation);
        if (input != stdin)
            fclose(input);
        if (output != stdout)
            fclose(output);
        return 1;
    }

    if (input != stdin)
        fclose(input);
    if (output != stdout)
        fclose(output);

    return 0;
}
