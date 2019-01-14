#include <stdlib.h>
#include "utils.h"

void initRandom(unsigned int seed) {
    srand(seed);
}

int getRandomRange(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}
