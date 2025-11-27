#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "mat.h"

int main() {
    FILE *fp;

    uint8_t A[MAT_SIZE][MAT_SIZE] = {
        { 0, 4, 8, 8, 9, 10, 5 },
        { 8, 1, 2, 9, 9, 8, 7 },
        { 1, 8, 4, 10, 5, 4, 4 },
        { 10, 4, 8, 5, 3, 2, 7 },
        { 0, 4, 7, 4, 0, 1, 9 },
        { 0, 3, 9, 10, 3, 1, 9 },
        { 1, 7, 1, 9, 4, 0, 7 },
    };

    uint8_t B[MAT_SIZE][MAT_SIZE] = {
        { 69, 4, 8, 8, 9, 10, 5 },
        { 8, 69, 2, 9, 9, 8, 7 },
        { 1, 8, 4, 10, 5, 4, 4 },
        { 10, 4, 8, 5, 3, 2, 7 },
        { 0, 4, 7, 4, 0, 1, 9 },
        { 0, 3, 9, 10, 3, 1, 9 },
        { 1, 7, 1, 9, 4, 0, 69 },
    };

    uint8_t C[MAT_SIZE][MAT_SIZE];

    // Compute C = A + B*B
    mat_compute(A, B, C);

    // Write results to file
    fp = fopen("out.dat", "w");
    for (int i = 0; i < MAT_SIZE; i++) {
        for (int j = 0; j < MAT_SIZE; j++) {
            fprintf(fp, "%d ", C[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);

    // Compare against golden output
    printf("Comparing against output data\n");
    if (system("diff -w out.dat out.gold.dat")) {
        fprintf(stdout, "*******************************************\n");
        fprintf(stdout, "FAIL: Output DOES NOT match the golden output\n");
        fprintf(stdout, "*******************************************\n");
        return 1;
    } else {
        fprintf(stdout, "*******************************************\n");
        fprintf(stdout, "PASS: The output matches the golden output!\n");
        fprintf(stdout, "*******************************************\n");
        return 0;
    }
}
