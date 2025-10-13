// thread_safety.cpp
// Ejemplo práctico de función no thread-safe vs thread-safe
// Autor: Anthony
// Compilar: g++ -O2 -std=c++17 -pthread -o thread_safety thread_safety.cpp
#include <cstring>   //  para strchr(), strncpy()
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
using namespace std;

// ===========================================================
// ❌ Versión NO thread-safe: usa una variable estática compartida
// ===========================================================
char* my_strtok_not_safe(char* str, const char* delim) {
    static char* next;  // variable estática compartida entre hilos
    if (str) next = str;

    if (!next) return nullptr;

    // Buscar inicio del siguiente token
    char* start = next;
    while (*start && strchr(delim, *start)) start++;

    if (*start == '\0') { next = nullptr; return nullptr; }

    // Buscar final del token
    char* end = start;
    while (*end && !strchr(delim, *end)) end++;

    if (*end) {
        *end = '\0';
        next = end + 1;
    } else {
        next = nullptr;
    }
    return start;
}

//Versión thread-safe: sin variable compartida (usa stringstream)
vector<string> tokenize_safe(const string& line) {
    vector<string> tokens;
    istringstream iss(line);
    string word;
    while (iss >> word)
        tokens.push_back(word);
    return tokens;
}


// Función que simula tokenización en paralelo
void worker_not_safe(const string& text) {
    char buffer[256];
    strncpy(buffer, text.c_str(), sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    char* token = my_strtok_not_safe(buffer, " ");
    cout << "[Thread " << this_thread::get_id() << "] Tokens:";
    while (token) {
        cout << " (" << token << ")";
        token = my_strtok_not_safe(nullptr, " ");
    }
    cout << endl;
}

void worker_safe(const string& text) {
    auto tokens = tokenize_safe(text);
    cout << "[Thread " << this_thread::get_id() << "] Tokens:";
    for (auto& t : tokens) cout << " (" << t << ")";
    cout << endl;
}

// ===========================================================
// MAIN
// ===========================================================
int main() {
    vector<string> frases = {
        "Pease porridge hot",
        "Pease porridge cold",
        "Pease porridge in the pot",
        "Nine days old"
    };

    cout << "=== Ejemplo 1: NO THREAD-SAFE ===" << endl;
    {
        vector<thread> ths;
        for (auto& f : frases)
            ths.emplace_back(worker_not_safe, f);
        for (auto& t : ths) t.join();
    }

    cout << "\n=== Ejemplo 2: THREAD-SAFE ===" << endl;
    {
        vector<thread> ths;
        for (auto& f : frases)
            ths.emplace_back(worker_safe, f);
        for (auto& t : ths) t.join();
    }
    return 0;
}
