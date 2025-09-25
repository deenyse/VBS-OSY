#include <stdlib.h>
#include <string.h>
#include <time.h>
char *genWord()
{
    int word_length = rand() % 8 + 1;

    char *word = malloc(sizeof(char) * (word_length + 1));

    for (int i = 0; i < word_length; i++)
    {
        word[i] = rand() % 26;

        if (rand() % 2)
            word[i] += 'a';
        else
            word[i] += 'A';
    }

    word[word_length] = '\0';

    return word;
}

char *genLine(int words_amount)
{
    char *line = malloc(sizeof(char));
    line[0] = '\0';

    for (int i = 0; i < words_amount; i++)
    {
        char *word = genWord();
        line = realloc(line, sizeof(char) * (strlen(line) + strlen(word) + 2));

        strcat(word, " ");
        strcat(line, word);
    }

    return line;
}