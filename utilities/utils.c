#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

void initRandom(unsigned int seed) {
    srand(seed);
}

int getRandomRange(int min, int max) {
    return rand() % (max - min + 1) + min;
}

int getWeightedRand(int size, int *values, int *percentages) {
    int i, sum = 0, range;
    range = getRandomRange(1, 100);
    for (i = 0, sum = 0; i < size; i++) {
        printf("%d\n", i);
        fflush(stdout);
        sum += percentages[i];
        if (range < sum) {
            return values[i];
        }
    }

    return values[size - 1];
}
