#ifndef MAT_H
#define MAT_H

#include <stdint.h>

#define MAT_SIZE 7

// Top-level function: computes C = A + B*B
void mat_compute(uint8_t A[MAT_SIZE][MAT_SIZE], uint8_t B[MAT_SIZE][MAT_SIZE], uint8_t C[MAT_SIZE][MAT_SIZE]);

#endif
