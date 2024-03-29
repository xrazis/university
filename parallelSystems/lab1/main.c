#include <stdio.h>
#include <omp.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Calculate |𝛢𝑖𝑖| > ∑ |𝐴𝑖𝑗| where j=0…N-1 i<>j
bool strictlyDiagonallyDominant(int *sddArray, int sddArraySize, int chunk) {
    int i, j, sum;
    bool isSDD = true;

#pragma omp parallel shared(sddArray, sddArraySize, chunk, isSDD) private(i, j, sum) default(none)
    {
#pragma omp for schedule(static, chunk)
        for (i = 0; i < sddArraySize; i++) {
            sum = 0;

            for (j = 0; j < sddArraySize; j++) {
                if (i != j)
                    sum += abs(sddArray[i * sddArraySize + j]);
            }

            if (abs(sddArray[i * sddArraySize + i]) < sum) {
                isSDD = false;
            }
        }
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
    printf(isSDD ? "\nThe matrix is strictly diagonally dominant!\n"
                 : "\nThe matrix is not strictly diagonally dominant!\n");
#pragma clang diagnostic pop

    return isSDD;
}

// Calculate m = max(|𝛢𝑖𝑖|) where i=0…N-1
int maxInDiagonal(int *sddArray, int sddArraySize) {
    int i;
    int max = abs(sddArray[1 * sddArraySize + 1]);

#pragma omp parallel for private(i) shared(sddArraySize, sddArray) reduction(max:max) default(none)
    for (i = 0; i < sddArraySize; ++i) {
        {
            if (abs(sddArray[i * sddArraySize + i]) > max)
                max = abs(sddArray[i * sddArraySize + i]);
        }
    }

    printf("\nThe max element in the diagonal of the matrix is: %d\n", max);
    return max;
}

// Create new array where 𝐵𝑖𝑗 = m–|𝐴𝑖𝑗| for i<>j and 𝐵𝑖𝑗 = m for i=j
void createNewArray(int *sddArray, int *sddMaxArray, int sddArraySize, int chunk, int max) {
    int i, j;

#pragma omp parallel shared(sddArray, sddArraySize, chunk, sddMaxArray, max) private(i, j) default(none)
    {
#pragma omp for schedule(static, chunk)
        for (i = 0; i < sddArraySize; i++) {
            for (j = 0; j < sddArraySize; j++) {
                if (i != j) {
                    sddMaxArray[i * sddArraySize + j] = max - abs(sddArray[i * sddArraySize + j]);
                } else {
                    sddMaxArray[i * sddArraySize + j] = max;
                }
            }
        }
    }

    printf("\n-Created new array-\n");
}

// Calculate m = max(|𝛢𝑖𝑖|) where i=0…N-1 with reduction clause
void minInDiagonalWithReduction(int *sddMaxArray, int sddArraySize) {
    int i, j;
    int min = abs(sddMaxArray[1 * sddArraySize + 1]);

#pragma omp parallel for private(i, j) shared(sddArraySize, sddMaxArray) reduction(min:min) default(none)
    for (i = 0; i < sddArraySize; ++i) {
        for (j = 0; j < sddArraySize; ++j) {
            {
                if (abs(sddMaxArray[i * sddArraySize + j]) < min) {
                    min = abs(sddMaxArray[i * sddArraySize + j]);
                }
            }
        }
    }

    printf("\n-Calculated with reduction clause-\nThe min element in the matrix is: %d\n", min);
}

// Calculate m = max(|𝛢𝑖𝑖|) where i=0…N-1 with critical area
void minInDiagonalWithCriticalArea(int *sddMaxArray, int sddArraySize, int chunk) {
    int i, j;
    int min = abs(sddMaxArray[1 * sddArraySize + 1]);

#pragma omp parallel shared(sddArraySize, sddMaxArray, chunk, min) private(i, j) default(none)
    {
        int minLocal = min;

#pragma omp for schedule(static, chunk)
        for (i = 0; i < sddArraySize; ++i)
            for (j = 0; j < sddArraySize; ++j)
                if (abs(sddMaxArray[i * sddArraySize + j]) < minLocal)
                    minLocal = abs(sddMaxArray[i * sddArraySize + j]);;

#pragma omp critical
        {
            if (minLocal < min) min = minLocal;
        }

    }

    printf("\n-Calculated with critical area-\nThe min element in the matrix is: %d\n", min);
}

int main() {
    int i, j, numThreads, sddArraySize, chunk, max;
    int *sddArray, *sddMaxArray;
    double start, end, timeInMilliseconds;

    printf("-------------------------------\n");
    printf("Parallel Systems - Assignment 1\n");
    printf("-------------------------------\n\n");

    printf("What array size fits you? N=");
    scanf("%d", &sddArraySize);

    sddArray = (int *) malloc(sddArraySize * sddArraySize * sizeof(int));
    sddMaxArray = (int *) malloc(sddArraySize * sddArraySize * sizeof(int));

    printf("\nEnter number of threads for OpenMP: ");
    scanf("%d", &numThreads);

    printf("\nSetting %d threads", numThreads);
    omp_set_num_threads(numThreads);

    for (i = 0; i < sddArraySize; i++) {
        for (j = 0; j < sddArraySize; j++) {
            printf("\nGive element [%d][%d]=", i, j);
            scanf("%d", &sddArray[i * sddArraySize + j]);
        }
    }

    chunk = sddArraySize / numThreads;
    if (chunk == 0) chunk = 1;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"

    // Measure performance
    start = omp_get_wtime();

    if (!strictlyDiagonallyDominant(sddArray, sddArraySize, chunk)) {
        exit(1);
    }

    max = maxInDiagonal(sddArray, sddArraySize);

    createNewArray(sddArray, sddMaxArray, sddArraySize, chunk, max);

    minInDiagonalWithReduction(sddMaxArray, sddArraySize);

    minInDiagonalWithCriticalArea(sddMaxArray, sddArraySize, chunk);

    end = omp_get_wtime();
    timeInMilliseconds = (end - start) * 1000;

    printf("\n-----------------------------------------\n");
    printf("Functions executed in %.4f milliseconds\n", timeInMilliseconds);
    printf("-----------------------------------------\n");

    FILE *f = fopen("results.txt", "a");
    if (f == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }

    fprintf(f, "N: %d\t Threads: %d\t Time: %.4f ms\n", sddArraySize, numThreads, timeInMilliseconds);

    fclose(f);
    free(sddArray);
    free(sddMaxArray);

    return 0;
#pragma clang diagnostic pop
}
