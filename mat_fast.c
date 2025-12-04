#include "mat.h"

// FAST VERSION: Maximum optimization using combined techniques
// - Array partitioning for parallel memory access
// - Pipeline on outer loops with II=1
// - Full unroll on inner loops
// Computes C = A + B*B

void mat_compute(uint8_t A[MAT_SIZE][MAT_SIZE], uint8_t B[MAT_SIZE][MAT_SIZE], uint8_t C[MAT_SIZE][MAT_SIZE]) {
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE s_axilite port=A
#pragma HLS INTERFACE s_axilite port=B
#pragma HLS INTERFACE s_axilite port=C

#pragma HLS ARRAY_PARTITION variable=A complete dim=2
#pragma HLS ARRAY_PARTITION variable=B complete dim=0
#pragma HLS ARRAY_PARTITION variable=C complete dim=2

    uint8_t temp[MAT_SIZE][MAT_SIZE];
#pragma HLS ARRAY_PARTITION variable=temp complete dim=0

    int i, j, k;

    // temp = B * B
    for (i = 0; i < MAT_SIZE; i++) {
#pragma HLS PIPELINE
        for (j = 0; j < MAT_SIZE; j++) {
#pragma HLS UNROLL
            uint8_t sum = 0;
            for (k = 0; k < MAT_SIZE; k++) {
#pragma HLS UNROLL
                sum += B[i][k] * B[k][j];
            }
            temp[i][j] = sum;
        }
    }

    // C = A + temp
    for (i = 0; i < MAT_SIZE; i++) {
#pragma HLS PIPELINE
        for (j = 0; j < MAT_SIZE; j++) {
#pragma HLS UNROLL
            C[i][j] = A[i][j] + temp[i][j];
        }
    }
}
