/*
 * Estimación de PI - Versión Secuencial
 * 
 * Compilación: g++ -g -Wall -o pi_seq pi_seq.cpp
 * Ejecución:   ./pi_seq <número_de_terminos>
 * Ejemplo:     ./pi_seq 100000000
 */

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <chrono>

using namespace std;
using namespace chrono;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <número_de_terminos>\n";
        exit(1);
    }

    long long n = atoll(argv[1]);
    double sum = 0.0;

    auto start = high_resolution_clock::now();

    for (long long i = 0; i < n; i++) {
        double factor = (i % 2 == 0) ? 1.0 : -1.0;
        sum += factor / (2 * i + 1);
    }

    double pi_estimate = 4.0 * sum;

    auto end = high_resolution_clock::now();
    auto elapsed = duration_cast<duration<double>>(end - start);

    cout.precision(15);
    cout << "Estimación de π: " << pi_estimate << endl;
    cout << "Tiempo de ejecución: " << elapsed.count() << " segundos" << endl;

    return 0;
}
