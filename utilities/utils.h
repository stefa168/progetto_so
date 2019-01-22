#ifndef PROGETTO_SO_UTILS_H
#define PROGETTO_SO_UTILS_H

void initRandom(unsigned int seed);

int getRandomRange(int min, int max);

int getWeightedRand(int size, int values[], int percentages[]);

int getWeighted(int size, int value, int percCount, int *values, int *percentages);

#endif
