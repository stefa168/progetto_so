#include <stdlib.h>
#include "utils.h"

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
        sum += percentages[i];
        if (range < sum) {
            return values[i];
        }
    }

    return values[size - 1];
}

int getWeighted(int size, int value, int percCount, int *values, int *percentages) {
    if (value < 0) {
        return values[0];
    }
    int i, sum;

    for (i = 0, sum = 0; i < percCount; i++) {
        sum += (int) (size * percentages[i] / (float) 100);
        if (sum > value) {
            return values[i];
        }
    }

    return values[percCount - 1];
}
