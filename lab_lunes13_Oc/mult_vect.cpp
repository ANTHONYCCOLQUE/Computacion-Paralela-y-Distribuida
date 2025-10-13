// matvec_mt.cpp
// Multiplicación matriz–vector multithread (implementación propia)
// Autor: Anthony
// Compilar: g++ -O2 -std=c++17 -pthread -o matvec_mt matvec_mt.cpp
//
// Uso:
//   ./matvec_mt <n_filas> <n_columnas> <threads>
// Ejemplo:
//   ./matvec_mt 2000 2000 4

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

using namespace std;
using namespace chrono;

// ===== Estructura de datos =====
struct Task {
    const vector<vector<double>>* A;
    const vector<double>* x;
    vector<double>* y;
    int start_row;
    int end_row;
};

// ===== Función de cada hilo =====
void worker(Task t) {
    for (int i = t.start_row; i < t.end_row; ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < t.x->size(); ++j)
            sum += (*(t.A))[i][j] * (*(t.x))[j];
        (*(t.y))[i] = sum;
    }
}

// ===== Programa principal =====
int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Uso: " << argv[0] << " <filas> <columnas> <threads>\n";
        return 1;
    }

    int n = stoi(argv[1]); // filas
    int m = stoi(argv[2]); // columnas
    int thread_count = stoi(argv[3]);

    cout << "Matriz: " << n << "x" << m << ", Threads: " << thread_count << "\n";

    // --- Inicializar matriz y vector ---
    vector<vector<double>> A(n, vector<double>(m));
    vector<double> x(m), y(n);

    mt19937 gen(42);
    uniform_real_distribution<double> dist(0.0, 1.0);

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            A[i][j] = dist(gen);

    for (int j = 0; j < m; ++j)
        x[j] = dist(gen);

    // --- Multiplicación paralela ---
    vector<thread> threads;
    vector<Task> tasks(thread_count);

    int rows_per_thread = n / thread_count;
    int remainder = n % thread_count;

    auto start = high_resolution_clock::now();

    int current = 0;
    for (int t = 0; t < thread_count; ++t) {
        int start_row = current;
        int end_row = start_row + rows_per_thread + (t < remainder ? 1 : 0);
        current = end_row;

        tasks[t] = { &A, &x, &y, start_row, end_row };
        threads.emplace_back(worker, tasks[t]);
    }

    for (auto& th : threads) th.join();

    auto end = high_resolution_clock::now();
    double time_ms = duration<double, milli>(end - start).count();

    cout << "Tiempo total: " << time_ms / 1000.0 << " s\n";
    cout << "Primeros 5 valores de y: ";
    for (int i = 0; i < min(5, n); ++i) cout << y[i] << " ";
    cout << "\n";

    return 0;
}
