#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"

/* Exit codes
1 - file open
2 - memory allocation error
*/

// Bad global variable
static bool had_error = false;

// Good practise to separate the code that generates the errors from the code that reports them.
// TODO: Best way it's create "ErrorReporter" interface that gets passed to scanner and parser
void report(int line, char *where, char *message) {
    fprintf(stderr, "[line %d] Error%s: %s", line, where, message);
    had_error = true;
}
void error(int line, char *message) {
    report(line, "", message);
}


void run(char *program, size_t n) {
    printf("%s", program);
    // Smth like that
    /*
    Scanner scanner = malloc(sizeof(Scanner));
    vec<Token> tokens = scanner.scan_tokens();

    for (Token token : tokens) {
        printf("%s\n", token);
    }
    */
}

// Execute line by line
void run_prompt() {
    char line[1024];

    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL)
            continue;

        size_t n = strcmp(line, "\n");
        if (n == 0) break;
        
        run(line, n);
        had_error = false;
    }
}

// run full file
void run_file(char *path) {
    FILE *file = fopen(path, "rb");
    file = fopen(path, "r");
    if (!file) {
        printf("Can't execute file.\n");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);  // move to start

    char *buffer = malloc(file_size); // * sizeof(char)
    if (!buffer) {
        printf("Memory allocation error.\n");
        fclose(file);
        exit(2);
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read != file_size) {
        printf("Error while reading a file.\n");
        free(buffer);
        fclose(file);
        exit(1);
    }

    run(buffer, file_size);
    if (had_error) exit(65);

    free(buffer);
    fclose(file);
}

int main(int argc, char **argv) {
    if (argc > 2) {
        printf("Usage: jlox [script]");
        exit(64);
    } else if (argc == 2) { // if file in argv
        run_file(argv[1]);
    } else {                // without argv
        run_prompt();
    }
    return 0;
}