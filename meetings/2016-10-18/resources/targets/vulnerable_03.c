// Goal: Have function bar print out your name!

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void bar(char* string) {
    printf(string);
    return;
}

void foo(char* string) {
    char buffer[32];
    strcpy(buffer, string);
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