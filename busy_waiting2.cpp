/*
 * Estimación de π - Pthreads con Busy-Waiting FUERA del bucle FOR
 * ✅ Incluye salida para demostrar el control de turno al final (no en cada iteración)
 * 
 * Compilar: g++ -g -Wall -o busy_waiting2 busy_waiting2.cpp -lpthread
 * Ejecutar: ./busy_waiting2 <num_threads> <num_terminos>
 */

#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <cmath>
#include <chrono>

using namespace std;
using namespace chrono;

// Variables globales compartidas
long long n;
int thread_count;
double sum = 0.0;
int flag = 0;  // turno de los hilos

void* Thread_sum(void* rank) {
    long my_rank = (long) rank;
    long long my_n = n / thread_count;
    long long my_first_i = my_rank * my_n;
    long long my_last_i = my_first_i + my_n;

    // 1. Cada hilo calcula su suma local sin interferencia
    double my_sum = 0.0;
    for (long long i = my_first_i; i < my_last_i; i++) {
        double factor = (i % 2 == 0) ? 1.0 : -1.0;
        my_sum += factor / (2 * i + 1);
    }

    printf("🧮 Hilo %ld terminó su suma local: %.6f\n", my_rank, my_sum);

    // 2. Busy-waiting FUERA del bucle: espera una sola vez su turno para actualizar sum
    while (flag != my_rank) {
        printf("🕒 Hilo %ld esperando turno para actualizar sum (flag = %d)\n", my_rank, flag);
    }

    // 3. Sección crítica: actualiza la suma global
    sum += my_sum;
    printf("✅ Hilo %ld actualizó sum: %.6f → nueva sum = %.6f\n", my_rank, my_sum, sum);

    // 4. Cede el turno
    flag = (flag + 1) % thread_count;

    printf("✅✅ Hilo %ld terminó\n", my_rank);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <num_threads> <num_terminos>\n";
        return 1;
    }

    thread_count = atoi(argv[1]);
    n = atoll(argv[2]);

    pthread_t* thread_handles = new pthread_t[thread_count];

    auto start = high_resolution_clock::now();

    for (long thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL, Thread_sum, (void*) thread);

    for (int thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);

    auto end = high_resolution_clock::now();
    auto elapsed = duration_cast<duration<double>>(end - start);

    double pi_estimate = 4.0 * sum;
    cout.precision(15);
    cout << "\n🔢 Estimación de π: " << pi_estimate << endl;
    cout << "⏱️  Tiempo de ejecución: " << elapsed.count() << " segundos" << endl;
    cout << "🎯 Valor real de π: " << M_PI << endl;

    delete[] thread_handles;
    return 0;
}
