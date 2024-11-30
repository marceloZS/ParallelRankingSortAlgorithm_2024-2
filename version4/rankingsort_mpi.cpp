#include <mpi.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <map>

// Funciones de comunicación modularizadas
void performGossip(const std::vector<int>& data, std::vector<int>& all_data, MPI_Comm comm) {
    MPI_Allgather(data.data(), data.size(), MPI_INT, all_data.data(), data.size(), MPI_INT, comm);
}

void performBroadcast(std::vector<int>& data, int root, MPI_Comm comm) {
    MPI_Bcast(data.data(), data.size(), MPI_INT, root, comm);
}

void performSort(std::vector<int>& data) {
    std::sort(data.begin(), data.end());
}

void performLocalRanking(const std::vector<int>& data, std::vector<int>& ranks) {
    for (size_t i = 0; i < data.size(); ++i) {
        ranks[i] = i + 1;
    }
}

void performReduce(const std::vector<int>& local_data, std::vector<int>& reduced_data, int root, MPI_Comm comm) {
    MPI_Reduce(local_data.data(), reduced_data.data(), local_data.size(), MPI_INT, MPI_SUM, root, comm);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int array_size = SIZE;
    std::vector<int> unsorted_array(array_size);

    if (rank == 0) {
        std::srand(std::time(nullptr)); // Seed for random number generation
        std::cout << "Creating Random List of " << array_size << " elements\n";
        for (int j = 0; j < array_size; ++j) {
            unsorted_array[j] = std::rand() % 1000; // Random numbers between 0 and 999
        }
        std::cout << "Created\n";
    }

    // Broadcast the array to all processes
    performBroadcast(unsorted_array, 0, MPI_COMM_WORLD);

    // Example usage of Gossip and Reduce
    std::vector<int> all_data(size * array_size);
    performGossip(unsorted_array, all_data, MPI_COMM_WORLD);

    std::vector<int> reduced_data(array_size);
    performReduce(unsorted_array, reduced_data, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Reduced data received\n";
    }

    MPI_Finalize();
    return 0;
}