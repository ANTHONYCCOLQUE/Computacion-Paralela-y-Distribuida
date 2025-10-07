/*
 * Estimación de π - Pthreads con Busy-Waiting DENTRO del bucle FOR (versión corregida)
 * 💬 Incluye salida para visualizar la espera activa (busy-waiting)
 * 
 * Compilar: g++ -g -Wall -o busy_waiting1 busy_waiting1.cpp -lpthread
 * Ejecutar: ./busy_waiting1 <num_threads> <num_terminos>
 */

#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <cmath>
#include <chrono>

using namespace std;
using namespace chrono;

// Variables globales compartidas
long long n;              // Total de términos
int thread_count;         // Número de hilos
double sum = 0.0;         // Suma global
int flag = 0;             // Bandera para controlar turno

void* Thread_sum(void* rank) {
    long my_rank = (long) rank;
    long long my_n = n / thread_count;
    long long my_first_i = my_rank * my_n;
    long long my_last_i = my_first_i + my_n;

    for (long long i = my_first_i; i < my_last_i; i++) {
        // Espera activa (busy-waiting) en cada iteración ANTES de calcular
        while (flag != my_rank) {
            // Mostrar espera activa cada 50,000 iteraciones (ajustable)
            if (i % 50000 == 0) {
                printf("🕒 Hilo %ld esperando turno en i = %lld (flag = %d)\n",
                       my_rank, i, flag);
            }
        }

        // Una vez que le toca su turno, calcula el término y actualiza sum
        double factor = (i % 2 == 0) ? 1.0 : -1.0;
        double term = factor / (2 * i + 1);
        sum += term;

        // Mostrar suma de término cada 50,000 iteraciones
        if (i % 50000 == 0) {
            printf("✅ Hilo %ld sumó término i = %lld → sum = %.6f\n",
                   my_rank, i, sum);
        }

        // Pasa el turno al siguiente hilo
        flag = (flag + 1) % thread_count;
    }

    // Fin del hilo
    printf("✅✅ Hilo %ld terminó sus %lld iteraciones\n",
           my_rank, my_last_i - my_first_i);
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
