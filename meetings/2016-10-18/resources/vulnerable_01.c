#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void foo(char* string) {
    char buffer[32];
    printf("address of buffer: %p\n", buffer);
    strcpy(buffer, string);
    printf("Copy successful!\n");
    return;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <STRING>\n", argv[0]);
        return 1;
    }

    printf("main is at address: %p\n", &main);

    foo(argv[1]);
    printf("Exiting...\n");
    return 0;
}