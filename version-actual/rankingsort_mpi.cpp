#include <mpi.h>
#include <math.h>
#include <ctime>
#include <cstdlib>  
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

void rank_sort(int *array, int *sorted, int n, int rank, int size) {
    int *rankings = (int *)malloc(n * sizeof(int));

    for (int i = rank; i < n; i += size) {
        rankings[i] = 0;
        for (int j = 0; j < n; j++) {
            if (array[j] < array[i] || (array[j] == array[i] && j < i)) {
                rankings[i]++;
            }
        }
    }

    MPI_Allreduce(MPI_IN_PLACE, rankings, n, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    for (int i = 0; i < n; i++) {
        sorted[rankings[i]] = array[i];
    }

    free(rankings);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size, n = 8;
    int array[8] = {8, 4, 2, 6, 1, 7, 3, 5};
    int sorted[8];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    rank_sort(array, sorted, n, rank, size);

    if (rank == 0) {
        printf("Sorted array: ");
        for (int i = 0; i < n; i++) {
            printf("%d ", sorted[i]);
        }
        printf("\n");
    }

    MPI_Finalize();
    return 0;
}
