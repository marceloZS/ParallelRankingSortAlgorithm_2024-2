#include <mpi.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int P = std::sqrt(size);  // size = P^2
    int n = 16;  // Total
    std::vector<int> d(n);
    if (rank == 0) {
        //  raíz
        for (int i = 0; i < n; ++i) d[i] = rand() % 100;
    }

    int N = n / size;  // Elements per process
    std::vector<int> local_d(N);

    // datos entre los procesos
    MPI_Scatter(d.data(), N, MPI_INT, local_d.data(), N, MPI_INT, 0, MPI_COMM_WORLD);

    // Gossip: datos dentro de cada columna
    std::vector<int> column_data(N * P);
    MPI_Allgather(local_d.data(), N, MPI_INT, column_data.data(), N, MPI_INT, MPI_COMM_WORLD);

    // Broadcast: datos dentro de cada fila
    int row = rank / P;
    int row_start = row * P;
    std::vector<int> row_data(N * P);
    MPI_Allgather(local_d.data(), N, MPI_INT, row_data.data(), N, MPI_INT, MPI_COMM_WORLD + row_start);

    // Sort localmente
    std::sort(row_data.begin(), row_data.end());

    // Local Ranking
    std::vector<int> local_rank(N * P);
    for (int i = 0; i < N * P; ++i) {
        local_rank[i] = i + 1;
    }

    // Reduce: rankings en el proceso raíz de cada fila
    std::vector<int> reduced_rank(N * P);
    MPI_Reduce(local_rank.data(), reduced_rank.data(), N * P, MPI_INT, MPI_SUM, row_start, MPI_COMM_WORLD);

    
    if (rank == row_start) {
        std::cout << "Reduced ranks in row " << row << ": ";
        for (int i = 0; i < N * P; ++i) {
            std::cout << reduced_rank[i] << " ";
        }
        std::cout << std::endl;
    }

    MPI_Finalize();
    return 0;
}