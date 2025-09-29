#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64

#include "parser.h"

char** parse_input(char* input) {
    char** tokens = malloc(MAX_TOKENS * sizeof(char*));
    if (!tokens) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    char* token;
    int position = 0;

    token = strtok(input, " \t\r\n\a");
    while (token != NULL && position < MAX_TOKENS - 1) {
        tokens[position] = token;
        position++;
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}
