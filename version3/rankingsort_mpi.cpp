#include <mpi.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <map>

// Utilidad para convertir strings a ints para ordenamiento uniforme
int convertDataToInt(const std::string& data) {
    static std::map<std::string, int> lookup;
    static int nextValue = 1;
    if (lookup.find(data) == lookup.end()) {
        lookup[data] = nextValue++;
    }
    return lookup[data];
}

// Funciones de comunicación modularizadas
void performGossip(std::vector<int>& data, int P, int rank) {
    std::vector<int> column_data(data.size() * P);
    MPI_Allgather(data.data(), data.size(), MPI_INT, column_data.data(), data.size(), MPI_INT, MPI_COMM_WORLD);
}

void performBroadcast(std::vector<int>& data, int P, int rank) {
    int row = rank / P;
    int row_start = row * P;
    std::vector<int> row_data(data.size() * P);
    MPI_Allgather(data.data(), data.size(), MPI_INT, row_data.data(), data.size(), MPI_INT, MPI_COMM_WORLD + row_start);
}

void performSort(std::vector<int>& data) {
    std::sort(data.begin(), data.end());
}

void performLocalRanking(const std::vector<int>& data, std::vector<int>& ranks) {
    for (size_t i = 0; i < data.size(); ++i) {
        ranks[i] = i + 1;
    }
}

void performReduce(std::vector<int>& local_rank, std::vector<int>& reduced_rank, int P, int rank) {
    int row = rank / P;
    int row_start = row * P;
    MPI_Reduce(local_rank.data(), reduced_rank.data(), local_rank.size(), MPI_INT, MPI_SUM, row_start, MPI_COMM_WORLD);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int P = std::sqrt(size);  // Asumimos que size = P^2
    int n = 16;  // Total de elementos
    std::vector<std::string> d_str = {"apple", "banana", "cherry", "date", "fig", "grape", "honeydew", "kiwi", "lemon", "mango", "nectarine", "orange", "peach", "quince", "raspberry", "strawberry"};
    std::vector<int> d(n);
    std::transform(d_str.begin(), d_str.end(), d.begin(), convertDataToInt);

    int N = n / size;  // Elementos por proceso
    std::vector<int> local_d(N);

    // Distribuir los datos entre los procesos
    MPI_Scatter(d.data(), N, MPI_INT, local_d.data(), N, MPI_INT, 0, MPI_COMM_WORLD);

    // Ejecutar las funciones de comunicación y procesamiento
    performGossip(local_d, P, rank);
    performBroadcast(local_d, P, rank);
    performSort(local_d);
    std::vector<int> local_rank(N);
    performLocalRanking(local_d, local_rank);
    std::vector<int> reduced_rank(N);
    performReduce(local_rank, reduced_rank, P, rank);

    // Imprimir los resultados en el proceso raíz de cada fila
    if (rank == 0) {
        std::cout << "Final reduced ranks: ";
        for (int i = 0; i < N; ++i) {
            std::cout << reduced_rank[i] << " ";
        }
        std::cout << std::endl;
    }

    MPI_Finalize();
    return 0;
}