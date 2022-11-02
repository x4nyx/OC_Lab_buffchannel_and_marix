#include <cstdlib>
#include <chrono>
#include <thread>
#include <map>
#include <iostream>
#include <vector>
#include "buffered_channel.h"


void makeMatrix(int**& a, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            a[i][j] = rand() % 10;
        }
    }
}


void printMatrix(int** a, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << a[i][j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}


void multiplyByBlocks(int** a, int** b, int** c, int n, int block,
    std::pair<int, int>& pair) {
    for (int i = pair.first; i < std::min(n, pair.first + block); ++i) {
        for (int j = pair.second; j < std::min(n, pair.second + block); ++j) {
            c[i][j] = 0;
            for (int k = 0; k < n; ++k) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

void channelMultiply(int** a, int** b, int**& c,
    int N, int bSize, BufferedChannel<std::pair<int, int>>& channel) {
    std::pair<std::pair<int, int>, bool> pair = channel.recv();
    while (pair.second) {
        multiplyByBlocks(a, b, c, N, bSize, pair.first);
        pair = channel.recv();
    }
}

void fullMultiply(int** a, int** b, int** c, int N, int bSize, int thrCount) {
    std::vector<std::thread> threads;
    int numB = N % bSize == 0 ? N / bSize : N / bSize + 1;
    BufferedChannel<std::pair<int, int>> channel(pow(numB, 2));
    for (int i = 0; i < N; i += bSize) {
        for (int j = 0; j < N; j += bSize) {
            std::pair<int, int> pair(i, j);
            channel.send(std::move(pair));
        }
    }
    channel.close();
    for (int i = 0; i < thrCount; ++i) {
        threads.emplace_back(channelMultiply, std::ref(a), std::ref(b), std::ref(c), N,
            bSize, std::ref(channel));
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

void calculate(int** a, int** b, int** c, int N) {
    for (int i = 1; i <= N; i++) {
        auto start = std::chrono::steady_clock::now();
        fullMultiply(a, b, c, N, i, 10);
        auto end = std::chrono::steady_clock::now();
        int time1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "Size of blocks = " << i << ";\nDuration of multiplying = " << time1 << " ms;\n";
        printMatrix(c, N);
    }
}

int main() {
    int N = 10;// Размер матрицы

    auto** a = new int* [N];
    auto** b = new int* [N];
    auto** c = new int* [N];

    for (int i = 0; i < N; i++) {
        a[i] = new int[N];
        b[i] = new int[N];
        c[i] = new int[N];
    }

    std::srand(time(0));

    makeMatrix(a, N);
    makeMatrix(b, N);
    makeMatrix(c, N);
    printMatrix(a, N);
    printMatrix(b, N);

    calculate(a, b, c, N);
    return 0;
}