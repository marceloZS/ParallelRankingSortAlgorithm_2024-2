#include <map>
#include <mpi.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <random>
#include <iomanip>
#include <fstream>
#include <iterator>
using namespace std;

float startTime, endTime;
float tiempoComputoInicio, tiempoComputoFin, tiempoComunicacionInicio, tiempoComunicacionFin;
float timePoints[8];

string retrieveString(size_t length) {
    ifstream file("characters.txt"); 
    string chars;

    if (file.is_open()) {
        getline(file, chars); 
        file.close();
    } else {
        cerr << "Error opening characters.txt" << endl;
        return ""; 
    }

    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<size_t> distribution(0, chars.size() - 1);

    string result;
    result.reserve(length);

    generate_n(back_inserter(result), length, [&]() {
        return chars[distribution(generator)];
    });

    return result;
}

string mergeData(const map<int, string>& dataMap) {
    string combined;
    for (const auto& entry : dataMap) {
        combined += entry.second;
    }
    return combined;
}

vector<int> computeLocalRank(const string& localData, const string& fullData) {
    // para contar la frecuencia de cada carácter en localData
    std::map<char, int> frequencyMap;
    for (char c : localData) {
        frequencyMap[c]++;
    }

    // para acumular las frecuencias de todos los caracteres menores o iguales a cada carácter
    std::map<char, int> cumulativeFrequency;
    int runningTotal = 0;
    for (auto& pair : frequencyMap) {
        runningTotal += pair.second;
        cumulativeFrequency[pair.first] = runningTotal;
    }

    // para almacenar los rangos locales
    std::vector<int> ranks(fullData.size(), 0);

    // rango local para cada carácter en fullData
    for (size_t i = 0; i < fullData.size(); i++) {
        char currentChar = fullData[i];
        // carácter actual en el mapa de frecuencias acumuladas
        auto it = cumulativeFrequency.upper_bound(currentChar);
        if (it != cumulativeFrequency.begin()) {
            --it;
            ranks[i] = it->second;
        }
    }

    return ranks;
}

void performGossip(int rank, int rows, int cols, int size, map<int, string>& data) {
    int row = rank / cols;
    int col = rank % cols;
    char buffer[10000];

    for (int step = 0; step < rows - 1; step++) {
        int target = ((row + step + 1) % rows) * cols + col;  
        int source = ((row + rows - step - 1) % rows) * cols + col;  

        string toSend = mergeData(data);
        int sendSize = toSend.size() + 1;

        MPI_Sendrecv(toSend.c_str(), sendSize, MPI_CHAR, target, 0,
                     buffer, 10000, MPI_CHAR, source, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        data[(rank - cols + size) % size] = string(buffer);  
    }
}

void broadcastReverse(int rank, int rows, int cols, const string& data, map<int, string>& results) {
    int row = rank / cols;
    int col = rank % cols;
    char buffer[10000];

    if (col == row) {
        for (int c = 0; c < cols; c++) {
            if (c != col) {
                MPI_Send(data.c_str(), data.size() + 1, MPI_CHAR, row * cols + c, 0, MPI_COMM_WORLD);
            }
        }
        results[0] = data;
    } else {
        MPI_Recv(buffer, 10000, MPI_CHAR, row * cols + row, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        results[0] = string(buffer);
    }
}

string sortAndDisplayRanks(const vector<int>& ranks, const string& data) {
    vector<pair<int, char>> indexedRanks;

    for (size_t i = 0; i < data.size(); i++) {
        indexedRanks.emplace_back(ranks[i], data[i]);
    }
    sort(indexedRanks.begin(), indexedRanks.end(), [](const pair<int, char>& a, const pair<int, char>& b) {
        return a.first < b.first || (a.first == b.first && a.second < b.second);
    });

    string sortedData;
    for (const auto& rank : indexedRanks) {
        sortedData += rank.second;
    }

    return sortedData;
}

string processAndRankData(int rank, int rows, int cols, const string& initialData, const string& processedData) {
    string sortedData = initialData;

    tiempoComputoInicio = MPI_Wtime();
    sort(sortedData.begin(), sortedData.end());
    tiempoComputoFin = MPI_Wtime();

    vector<int> localRanks = computeLocalRank(sortedData, processedData);

    MPI_Barrier(MPI_COMM_WORLD);

    int row = rank / cols;
    int col = rank % cols;
    int diagProc = row * cols + row;
    char buffer[10000];
    string combinedData;

    tiempoComunicacionInicio = MPI_Wtime();
    if (col != row) {
        MPI_Send(localRanks.data(), localRanks.size(), MPI_INT, diagProc, 0, MPI_COMM_WORLD);
    } else {
        vector<int> totalRanks(localRanks.size(), 0);
        for (int c = 0; c < cols; c++) {
            if (c != col) {
                vector<int> receivedRanks(localRanks.size());
                MPI_Recv(receivedRanks.data(), receivedRanks.size(), MPI_INT, row * cols + c, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (size_t i = 0; i < totalRanks.size(); ++i) {
                    totalRanks[i] += receivedRanks[i];
                }
            } else {
                for (size_t i = 0; i < totalRanks.size(); i++) {
                    totalRanks[i] += localRanks[i];
                }
            }
        }

        if (rank != 0) {
            MPI_Send(totalRanks.data(), totalRanks.size(), MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(processedData.c_str(), processedData.size() + 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
        } else {
            vector<int> globalRanks(totalRanks);

            for (int r = 1; r < rows; r++) {
                int diagProc = r * cols + r;
                vector<int> receivedRanks(totalRanks.size());
                MPI_Recv(receivedRanks.data(), receivedRanks.size(), MPI_INT, diagProc, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(buffer, 10000, MPI_CHAR, diagProc, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                string receivedData(buffer);
                combinedData += receivedData;

                for (size_t i = 0; i < receivedRanks.size(); i++) {
                    globalRanks[i] += receivedRanks[i];
                }
            }

            sortedData = sortAndDisplayRanks(globalRanks, processedData + combinedData);
        }
    }
    tiempoComunicacionFin = MPI_Wtime();
    return sortedData;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int gridDim = static_cast<int>(sqrt(size));
    if (gridDim * gridDim != size) {
        if (rank == 0) cerr << "Error: Number of processes must be a perfect square." << endl;
        MPI_Finalize();
        return 1;
    }

    const int rows = gridDim;
    const int cols = gridDim;

    if (argc < 2) {
        if (rank == 0) cerr << "Usage: mpiexec -n <num_processes> ./program <message_size>" << endl;
        MPI_Finalize();
        return 1;
    }

    int msgSize = atoi(argv[1])/size;
    if (msgSize <= 0) {
        if (rank == 0) cerr << "Error: Message size must be a positive integer." << endl;
        MPI_Finalize();
        return 1;
    }

    int totalElements = rows * cols;
    string inputData;

    if (rank == 0) {
        inputData = retrieveString(msgSize * totalElements);
        if (inputData.size() % totalElements != 0) {
            cerr << "Input Size [" << inputData.size() << "] doesn't match row * col size [" << totalElements << "]" << endl;
            MPI_Finalize();
            return 1;
        }
    }

    char* localData = new char[msgSize + 1];
    localData[msgSize] = '\0';

    startTime = MPI_Wtime();

    timePoints[0] = MPI_Wtime();
    MPI_Scatter(inputData.c_str(), msgSize, MPI_CHAR, localData, msgSize, MPI_CHAR, 0, MPI_COMM_WORLD);
    timePoints[1] = MPI_Wtime();

    string localDataStr(localData);
    delete[] localData;

    map<int, string> dataMap = {{rank, localDataStr}};
    map<int, string> resultData;
    timePoints[2] = MPI_Wtime();
    performGossip(rank, rows, cols, size, dataMap);
    timePoints[3] = MPI_Wtime();
    string gossipResult = mergeData(dataMap);
    timePoints[4] = MPI_Wtime();
    broadcastReverse(rank, rows, cols, gossipResult, resultData);
    timePoints[5] = MPI_Wtime();

    timePoints[6] = MPI_Wtime();
    string finalResult = processAndRankData(rank, rows, cols, gossipResult, mergeData(resultData));
    timePoints[7] = MPI_Wtime();
    endTime = MPI_Wtime();

    if (rank == 0) {
        cout << fixed << setprecision(10);
        cout << "Execution Time: " << (endTime - startTime) << endl;
        cout << "Scatter Time: " << (timePoints[1] - timePoints[0]) << endl;
        cout << "Gossip Time: " << (timePoints[3] - timePoints[2]) << endl;
        cout << "Broadcast Time: " << (timePoints[5] - timePoints[4]) << endl;
        cout << "Process Time: " << (timePoints[7] - timePoints[6]) << endl;
        cout << "Tiempo de cómputo: " << (tiempoComputoFin - tiempoComputoInicio) << " segundos." << endl;
        cout << "Tiempo de comunicación: " << (tiempoComunicacionFin - tiempoComunicacionInicio) << " segundos." << endl;
    }

    MPI_Finalize();
    return 0;
}