// Goal: Open a shell!

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void foo(char* string) {
    char buffer[32];
    uint8_t size = strlen(string);

    if (size < sizeof buffer)
        strcpy(buffer, string);

    else 
        fprintf(stderr, "String is too big!\n");

    return;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <STRING>\n", argv[0]);
        return 1;
    }

    foo(argv[1]);
    return 0;
}