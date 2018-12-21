
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

    char *s;

    s = calloc(21, sizeof(char));

    printf("%s", s);

    return 0;
}