#include "mat.h"

// UNROLL VERSION: Inner loops fully unrolled for parallel execution
// Computes C = A + B*B

void mat_compute(uint8_t A[MAT_SIZE][MAT_SIZE], uint8_t B[MAT_SIZE][MAT_SIZE], uint8_t C[MAT_SIZE][MAT_SIZE]) {
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=A
#pragma HLS INTERFACE s_axilite port=B
#pragma HLS INTERFACE s_axilite port=C

    uint8_t temp[MAT_SIZE][MAT_SIZE];
    int i, j, k;

    // temp = B * B
    for (i = 0; i < MAT_SIZE; i++) {
        for (j = 0; j < MAT_SIZE; j++) {
            temp[i][j] = 0;
            for (k = 0; k < MAT_SIZE; k++) {
#pragma HLS UNROLL
                temp[i][j] += B[i][k] * B[k][j];
            }
        }
    }

    // C = A + temp
    for (i = 0; i < MAT_SIZE; i++) {
        for (j = 0; j < MAT_SIZE; j++) {
#pragma HLS UNROLL
            C[i][j] = A[i][j] + temp[i][j];
        }
    }
}
