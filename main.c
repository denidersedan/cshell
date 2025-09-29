#include <stdio.h>
#include <stdlib.h>

#include "include/parser.h"
#include "include/executor.h"
#include "include/history.h"
#include "include/input.h"

int main() {
    char* input;
    char** tokens;
    int status = 1; 
    load_history();
    input_init();
    while(status) {
        fflush(stdout);

        input = read_input_line("$ ");
        if (!input) { 
            printf("\n");
            break;
        }
        if (input[0] != '\0') add_history(input);
        tokens = parse_input(input);

        if (tokens[0] != NULL) {
            status = execute_command(tokens);
        }

        free(input);
        free(tokens);
    }
    save_history();
    input_restore();
    return 0;
}