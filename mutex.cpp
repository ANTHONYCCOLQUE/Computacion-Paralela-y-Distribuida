/*
 * Estimación de π - Pthreads con MUTEX
 * ✅ Visualización clara del uso de mutex para evitar race condition
 * 
 * Compilar: g++ -g -Wall -o pi_mutex_visual pi_mutex_visual.cpp -lpthread
 * Ejecutar: ./pi_mutex_visual <num_threads> <num_terminos>
 */

#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <cmath>
#include <chrono>

using namespace std;
using namespace chrono;

// Variables globales
long long n;
int thread_count;
double sum = 0.0;
pthread_mutex_t mutex;  // 🔒 Mutex global para proteger la suma

void* Thread_sum(void* rank) {
    long my_rank = (long) rank;
    long long my_n = n / thread_count;
    long long my_first_i = my_rank * my_n;
    long long my_last_i = my_first_i + my_n;

    // 1. Cálculo de suma parcial local
    double my_sum = 0.0;
    for (long long i = my_first_i; i < my_last_i; i++) {
        double factor = (i % 2 == 0) ? 1.0 : -1.0;
        my_sum += factor / (2 * i + 1);
    }

    printf("🧮 Hilo %ld terminó su suma local: %.6f\n", my_rank, my_sum);

    // 2. Intento de entrada a la sección crítica (protegida con mutex)
    printf("🕒 Hilo %ld esperando mutex para actualizar sum...\n", my_rank);
    pthread_mutex_lock(&mutex);
    printf("🔓 Hilo %ld obtuvo el mutex. sum += %.6f\n", my_rank, my_sum);

    // 3. Sección crítica
    sum += my_sum;

    pthread_mutex_unlock(&mutex);
    printf("✅ Hilo %ld liberó mutex y terminó. sum actual = %.6f\n", my_rank, sum);

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
    pthread_mutex_init(&mutex, NULL);  // Inicializa el mutex

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

    pthread_mutex_destroy(&mutex);  // Libera el mutex
    delete[] thread_handles;
    return 0;
}
