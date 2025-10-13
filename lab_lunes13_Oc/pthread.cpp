// lista_mt.cpp
// Lista enlazada ordenada de int (sin duplicados) con 3 estrategias: coarse, fine, rw.
// C++17, -pthread. Salida mínima: tiempos y resumen de operaciones.
//
// Compilar:
//   g++ -O2 -std=c++17 -pthread -o lista_mt lista_mt.cpp
//
// Uso:
//   ./lista_mt <strategy> <threads> <ops_por_thread> <member_pct> <insert_pct> <delete_pct> <init_n> <key_max> <seed>
//   strategy: coarse | fine | rw
//
// Ejemplos (como el PDF):
//   # 100000 ops/hilo, 99.9/0.05/0.05
//   ./lista_mt rw 8 100000 99.9 0.05 0.05 1000 100000 42
//   # 100000 ops/hilo, 80/10/10
//   ./lista_mt coarse 8 100000 80 10 10 1000 100000 42

#include <bits/stdc++.h>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
using namespace std;

// --------- utilidades ----------
struct Timer {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point t0;
    void start() { t0 = clock::now(); }
    double stop_ms() const {
        auto t1 = clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
};

struct IList {
    virtual bool Member(int key) = 0;
    virtual bool Insert(int key) = 0;
    virtual bool Delete(int key) = 0;
    virtual ~IList() = default;
};

// --------- Nodo base ----------
struct Node {
    int key;
    Node* next;
    explicit Node(int k): key(k), next(nullptr) {}
};

// --------- 1) coarse: mutex global ----------
class ListCoarse : public IList {
    Node* head{nullptr};
    mutable std::mutex m;
public:
    ~ListCoarse() override { clear(); }

    bool Member(int key) override {
        std::lock_guard<std::mutex> lk(m);
        Node* cur = head;
        while (cur && cur->key < key) cur = cur->next;
        return (cur && cur->key == key);
    }

    bool Insert(int key) override {
        std::lock_guard<std::mutex> lk(m);
        Node* pred = nullptr; Node* cur = head;
        while (cur && cur->key < key) { pred = cur; cur = cur->next; }
        if (cur && cur->key == key) return false;
        Node* n = new Node(key);
        if (!pred) { n->next = head; head = n; }
        else { n->next = cur; pred->next = n; }
        return true;
    }

    bool Delete(int key) override {
        std::lock_guard<std::mutex> lk(m);
        Node* pred = nullptr; Node* cur = head;
        while (cur && cur->key < key) { pred = cur; cur = cur->next; }
        if (!cur || cur->key != key) return false;
        if (!pred) head = cur->next; else pred->next = cur->next;
        delete cur; return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lk(m);
        Node* cur = head;
        while (cur) { Node* tmp = cur; cur = cur->next; delete tmp; }
        head = nullptr;
    }
};

// --------- 2) fine-grained: lock por nodo (lock coupling) ----------
struct FGNode {
    int key; FGNode* next; mutable std::mutex m;
    explicit FGNode(int k): key(k), next(nullptr) {}
};

class ListFine : public IList {
    FGNode* head{nullptr};
    mutable std::mutex head_m;
public:
    ~ListFine() override { clear(); }

    bool Member(int key) override {
        FGNode* pred = nullptr;
        FGNode* cur  = nullptr;
        {
            std::lock_guard<std::mutex> lk(head_m);
            cur = head;
            if (cur) cur->m.lock();
        }
        bool found = false;
        while (cur && cur->key < key) {
            if (pred) pred->m.unlock();
            pred = cur;
            cur = cur->next;
            if (cur) cur->m.lock();
        }
        if (cur && cur->key == key) found = true;
        if (cur) cur->m.unlock();
        if (pred) pred->m.unlock();
        return found;
    }

    bool Insert(int key) override {
        FGNode* pred = nullptr;
        FGNode* cur  = nullptr;
        {
            std::lock_guard<std::mutex> lk(head_m);
            cur = head;
            if (cur) cur->m.lock();
        }
        while (cur && cur->key < key) {
            if (pred) pred->m.unlock();
            pred = cur;
            cur = cur->next;
            if (cur) cur->m.lock();
        }
        if (cur && cur->key == key) {
            if (cur) cur->m.unlock();
            if (pred) pred->m.unlock();
            return false;
        }
        FGNode* n = new FGNode(key);
        n->next = cur;
        if (!pred) {
            std::lock_guard<std::mutex> lk(head_m);
            head = n;
        } else {
            pred->next = n;
        }
        if (cur) cur->m.unlock();
        if (pred) pred->m.unlock();
        return true;
    }

    bool Delete(int key) override {
        FGNode* pred = nullptr;
        FGNode* cur  = nullptr;
        {
            std::lock_guard<std::mutex> lk(head_m);
            cur = head;
            if (cur) cur->m.lock();
        }
        while (cur && cur->key < key) {
            if (pred) pred->m.unlock();
            pred = cur;
            cur = cur->next;
            if (cur) cur->m.lock();
        }
        if (!cur || cur->key != key) {
            if (cur) cur->m.unlock();
            if (pred) pred->m.unlock();
            return false;
        }
        if (!pred) {
            std::lock_guard<std::mutex> lk(head_m);
            head = cur->next;
        } else {
            pred->next = cur->next;
        }
        cur->m.unlock();
        if (pred) pred->m.unlock();
        delete cur; return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lk(head_m);
        FGNode* cur = head; head = nullptr;
        while (cur) { FGNode* tmp = cur; cur = cur->next; delete tmp; }
    }
};

// --------- 3) rw: shared_mutex global ----------
class ListRW : public IList {
    Node* head{nullptr};
    mutable std::shared_mutex rw;
public:
    ~ListRW() override { clear(); }

    bool Member(int key) override {
        std::shared_lock<std::shared_mutex> r(rw);
        Node* cur = head;
        while (cur && cur->key < key) cur = cur->next;
        return (cur && cur->key == key);
    }

    bool Insert(int key) override {
        std::unique_lock<std::shared_mutex> w(rw);
        Node* pred = nullptr; Node* cur = head;
        while (cur && cur->key < key) { pred = cur; cur = cur->next; }
        if (cur && cur->key == key) return false;
        Node* n = new Node(key);
        if (!pred) { n->next = head; head = n; }
        else { n->next = cur; pred->next = n; }
        return true;
    }

    bool Delete(int key) override {
        std::unique_lock<std::shared_mutex> w(rw);
        Node* pred = nullptr; Node* cur = head;
        while (cur && cur->key < key) { pred = cur; cur = cur->next; }
        if (!cur || cur->key != key) return false;
        if (!pred) head = cur->next; else pred->next = cur->next;
        delete cur; return true;
    }

    void clear() {
        std::unique_lock<std::shared_mutex> w(rw);
        Node* cur = head;
        while (cur) { Node* tmp = cur; cur = cur->next; delete tmp; }
        head = nullptr;
    }
};

// --------- helpers ----------
template <class L>
static void inicializar_lista(L& list, size_t n, int key_max, std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, key_max);
    size_t ins = 0; size_t intentos = 0;
    while (ins < n && intentos < n * 50) {
        if (list.Insert(dist(gen))) ++ins;
        ++intentos;
    }
}

struct Config {
    double p_member, p_insert, p_delete;
    int key_max;
};

struct Contadores {
    atomic<uint64_t> member_total{0}, insert_total{0}, delete_total{0};
    atomic<uint64_t> member_ok{0},     insert_ok{0},     delete_ok{0};
};

template <class L>
void trabajador(L* list, uint64_t ops, Config cfg, uint64_t seed, Contadores* c) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> pick(0.0,1.0);
    std::uniform_int_distribution<int> keys(0, cfg.key_max);

    for (uint64_t i = 0; i < ops; ++i) {
        double r = pick(rng);
        int k = keys(rng);
        if (r < cfg.p_member) {
            c->member_total++; if (list->Member(k)) c->member_ok++;
        } else if (r < cfg.p_member + cfg.p_insert) {
            c->insert_total++; if (list->Insert(k)) c->insert_ok++;
        } else {
            c->delete_total++; if (list->Delete(k)) c->delete_ok++;
        }
    }
}

// --------- main ----------
int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 10) {
        cerr << "Uso:\n"
             << "  " << argv[0] << " <strategy> <threads> <ops_por_thread> <member_pct> <insert_pct> <delete_pct> <init_n> <key_max> <seed>\n"
             << "  strategy: coarse | fine | rw\n";
        return 1;
    }

    string strategy = argv[1];
    int threads      = stoi(argv[2]);
    uint64_t ops_pt  = stoull(argv[3]);
    double m_pct     = stod(argv[4]);
    double i_pct     = stod(argv[5]);
    double d_pct     = stod(argv[6]);
    size_t init_n    = stoull(argv[7]);
    int key_max      = stoi(argv[8]);
    uint64_t seed    = stoull(argv[9]);

    if (fabs(m_pct + i_pct + d_pct - 100.0) > 1e-6) {
        cerr << "Error: los porcentajes deben sumar 100.\n";
        return 2;
    }

    Config cfg;
    cfg.p_member = m_pct / 100.0;
    cfg.p_insert = i_pct / 100.0;
    cfg.p_delete = d_pct / 100.0;
    cfg.key_max  = key_max;

    unique_ptr<IList> list;
    if (strategy == "coarse") list = make_unique<ListCoarse>();
    else if (strategy == "fine") list = make_unique<ListFine>();
    else if (strategy == "rw") list = make_unique<ListRW>();
    else { cerr << "Estrategia desconocida.\n"; return 3; }

    std::mt19937 gen(seed);
    inicializar_lista(*list, init_n, key_max, gen);

    vector<thread> pool;
    Contadores cont;
    Timer T; T.start();

    for (int t = 0; t < threads; ++t) {
        uint64_t s = seed + 101ULL * (t+1);
        pool.emplace_back(trabajador<IList>, list.get(), ops_pt, cfg, s, &cont);
    }
    for (auto& th : pool) th.join();

    double ms = T.stop_ms();
    uint64_t total_ops = ops_pt * (uint64_t)threads;

    cout << fixed << setprecision(3);
    cout << "=== Lista enlazada multithread (" << strategy << ") ===\n";
    cout << "Hilos: " << threads << ", Ops por hilo: " << ops_pt
         << " (Total: " << total_ops << ")\n";
    cout << "Mix: Member " << m_pct << "%, Insert " << i_pct << "%, Delete " << d_pct << "%\n";
    cout << "Init N: " << init_n << ", KeyMax: " << key_max << ", Seed: " << seed << "\n";
    cout << "Tiempo total: " << (ms/1000.0) << " s\n";
    cout << "Resultados: "
         << "Member " << cont.member_ok.load() << "/" << cont.member_total.load() << ", "
         << "Insert " << cont.insert_ok.load() << "/" << cont.insert_total.load() << ", "
         << "Delete " << cont.delete_ok.load() << "/" << cont.delete_total.load() << "\n";

    return 0;
}
