#include "../include/control_rw.hpp"
#include "../include/monitor_rw.hpp"
#include "../include/control_atomico_rw.hpp"
#include "../include/verificador.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <iomanip>
#include <stdexcept>
#include <ctime>

// Definición de la variable thread_local declarada en control_rw.hpp
thread_local int id_hebra = -1;

// Contadores de diagnóstico separados del estado de admisión (objetivo R2)
std::atomic<int> diag_lectoras_activas{0};
std::atomic<int> diag_escritoras_activas{0};
std::atomic<int> diag_max_lectoras{0};
std::atomic<long> diag_inconsistencias{0};
std::atomic<bool> diag_violacion_exclusion{false};

struct MetricasHebra {
    double suma_espera_us = 0.0;
    double max_espera_us = 0.0;
    EstadisticasAtomico estadisticas; // tendra estructura de tres contadores de fallos
};

// Actualiza diag_max_lectoras sin carreras de datos usando CAS (Compare & Swap)
void actualizar_max_lectoras(int actuales) {
    int prev_max = diag_max_lectoras.load(std::memory_order_relaxed);
    while (actuales > prev_max &&
           !diag_max_lectoras.compare_exchange_weak(prev_max, actuales, std::memory_order_relaxed)) {
    }
}

// Parseo estricto de enteros long (rechaza textos parciales como "12abc")
bool parsear_long(const std::string& str, long& salida) {
    try {
        size_t pos = 0;
        salida = std::stol(str, &pos);
        return pos == str.size();
    } catch (...) {
        return false;
    }
}

// Obtiene el tiempo de CPU del proceso en microsegundos (E2)
double obtener_cpu_us() {
    struct timespec ts{};
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == 0) {
        return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
    }
    return 0.0;
}

void tarea_lectora(int id, long K, long dur_lec_us, ControlRW* control, Registro& reg, MetricasHebra& metricas) {
    id_hebra = id;

    for (long i = 0; i < K; ++i) {
        auto t_ini = std::chrono::steady_clock::now();
        control->entrar_lectura();
        auto t_fin = std::chrono::steady_clock::now();

        double espera = std::chrono::duration<double, std::micro>(t_fin - t_ini).count();
        metricas.suma_espera_us += espera;
        if (espera > metricas.max_espera_us) metricas.max_espera_us = espera;

        // Diagnóstico al entrar (R2)
        int lect_act = diag_lectoras_activas.fetch_add(1, std::memory_order_seq_cst) + 1;
        if (diag_escritoras_activas.load(std::memory_order_seq_cst) > 0) {
            diag_violacion_exclusion.store(true, std::memory_order_relaxed);
        }
        actualizar_max_lectoras(lect_act);

        // Trabajo simulado fuera del control (R2)
        long v = reg.version;
        long dv = reg.doble_version;
        if (dv != 2 * v) {
            diag_inconsistencias.fetch_add(1, std::memory_order_relaxed);
        }
        if (dur_lec_us > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(dur_lec_us));
        }

        // Diagnóstico al salir (R2)
        diag_lectoras_activas.fetch_sub(1, std::memory_order_seq_cst);
        control->salir_lectura();
    }
    if (auto* atomico = dynamic_cast<ControlAtomicoRW*>(control)) { //estadisticas individuales de cada hebra
    metricas.estadisticas = atomico->obtener_estadisticas();
    }
}

void tarea_escritora(int id, long K, long dur_esc_us, ControlRW* control, Registro& reg, MetricasHebra& metricas) {
    id_hebra = id;

    for (long i = 0; i < K; ++i) {
        auto t_ini = std::chrono::steady_clock::now();
        control->entrar_escritura();
        auto t_fin = std::chrono::steady_clock::now();

        double espera = std::chrono::duration<double, std::micro>(t_fin - t_ini).count();
        metricas.suma_espera_us += espera;
        if (espera > metricas.max_espera_us) metricas.max_espera_us = espera;

        // Diagnóstico al entrar (R2)
        int esc_act = diag_escritoras_activas.fetch_add(1, std::memory_order_seq_cst) + 1;
        if (esc_act > 1 || diag_lectoras_activas.load(std::memory_order_seq_cst) > 0) {
            diag_violacion_exclusion.store(true, std::memory_order_relaxed);
        }

        // Trabajo simulado fuera del control (R2)
        reg.version++;
        reg.doble_version = 2 * reg.version;
        if (dur_esc_us > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(dur_esc_us));
        }

        // Diagnóstico al salir (R2)
        diag_escritoras_activas.fetch_sub(1, std::memory_order_seq_cst);
        control->salir_escritura();
    }
    if (auto* atomico = dynamic_cast<ControlAtomicoRW*>(control)) { //estadisticas individuales de cada hebra
        metricas.estadisticas = atomico->obtener_estadisticas();
    }
}

int main(int argc, char* argv[]) {
    // 1. Validar cantidad de argumentos (R1)
    if (argc != 8 && argc != 9) {
        std::cerr << "Uso: ./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]\n";
        return 1;
    }

    std::string modo = argv[1];
    std::string pol_str = argv[2];
    bool con_traza = true;

    if (argc == 9) {
        if (std::string(argv[8]) == "notraza") {
            con_traza = false;
        } else {
            std::cerr << "Error: el octavo argumento opcional solo puede ser 'notraza'.\n";
            return 1;
        }
    }

    // 2. Validar modo y política (R1)
    if (modo != "cond" && modo != "atomic") {
        std::cerr << "Error: modo invalido ('cond' o 'atomic').\n";
        return 1;
    }

    Politica politica;
    if (pol_str == "lectores") {
        politica = Politica::Lectores;
    } else if (pol_str == "escritores") {
        politica = Politica::Escritores;
    } else if (pol_str == "fifo") {
        politica = Politica::Fifo;
    } else {
        std::cerr << "Error: politica invalida ('lectores', 'escritores' o 'fifo').\n";
        return 1;
    }

    if (modo == "atomic" && politica != Politica::Escritores) {
        std::cerr << "Error: el modo 'atomic' solo soporta la politica 'escritores'.\n";
        return 1;
    }

    // 3. Parsear y validar rangos numéricos (R1)
    long L = 0, E = 0, K = 0, dur_lec_us = 0, dur_esc_us = 0;
    if (!parsear_long(argv[3], L) || !parsear_long(argv[4], E) ||
        !parsear_long(argv[5], K) || !parsear_long(argv[6], dur_lec_us) ||
        !parsear_long(argv[7], dur_esc_us)) {
        std::cerr << "Error: parametros numericos invalidos.\n";
        return 1;
    }

    if (L < 1 || E < 1 || (L + E) > 1000 || K < 1 || dur_lec_us < 0 || dur_esc_us < 0) {
        std::cerr << "Error: valores fuera de rango (L>=1, E>=1, L+E<=1000, K>=1, duraciones>=0).\n";
        return 1;
    }

    // 4. Validar que 2*E*K y (L+E)*K no desborden un long (R1)
    constexpr long MAX_LONG = std::numeric_limits<long>::max();
    if (E > MAX_LONG / 2 || K > MAX_LONG / (2 * E)) {
        std::cerr << "Error: 2*E*K desborda el tipo long.\n";
        return 1;
    }
    if (K > MAX_LONG / (L + E)) {
        std::cerr << "Error: (L+E)*K desborda el tipo long.\n";
        return 1;
    }

    // 5. Instanciar el controlador correspondiente (Sección 1)
    std::unique_ptr<ControlRW> control;
    MonitorRW* ptr_monitor = nullptr;
    ControlAtomicoRW* ptr_atomico = nullptr;

    if (modo == "cond") {
        auto mon = std::make_unique<MonitorRW>(politica, con_traza);
        ptr_monitor = mon.get();
        control = std::move(mon);
    } else {
        auto atm = std::make_unique<ControlAtomicoRW>();
        ptr_atomico = atm.get();
        control = std::move(atm);
    }

    Registro reg;
    std::vector<MetricasHebra> metricas(L + E);
    std::vector<std::thread> hebras;
    hebras.reserve(L + E);

    // 6. Iniciar relojes y lanzar hebras con manejo de errores (R1 y E2)
    auto pared_ini = std::chrono::steady_clock::now();
    double cpu_ini_us = obtener_cpu_us();

    try {
        for (int i = 0; i < L; ++i) {
            hebras.emplace_back(tarea_lectora, i, K, dur_lec_us, control.get(), std::ref(reg), std::ref(metricas[i]));
        }
        for (int j = 0; j < E; ++j) {
            int id = static_cast<int>(L) + j;
            hebras.emplace_back(tarea_escritora, id, K, dur_esc_us, control.get(), std::ref(reg), std::ref(metricas[id]));
        }
    } catch (const std::system_error& e) {
        std::cerr << "Error fatal al crear las hebras: " << e.what() << "\n";
        for (auto& h : hebras) {
            if (h.joinable()) h.join();
        }
        return 1;
    }

    for (auto& h : hebras) {
        h.join();
    }

    EstadisticasAtomico estadisticas_totales; //R10
    for (const auto& m : metricas) {
        estadisticas_totales.fallos_ocupado += m.estadisticas.fallos_ocupado;
        estadisticas_totales.fallos_escritores_esperando += m.estadisticas.fallos_escritores_esperando;
        estadisticas_totales.fallos_cas += m.estadisticas.fallos_cas;
        }

    double cpu_fin_us = obtener_cpu_us();
    auto pared_fin = std::chrono::steady_clock::now();

    // 7. Calcular métricas de tiempo y verificaciones finales (R2, R6, E2)
    double tiempo_pared_ms = std::chrono::duration<double, std::milli>(pared_fin - pared_ini).count();
    double tiempo_cpu_ms = (cpu_fin_us - cpu_ini_us) / 1000.0;

    double suma_esp_lec = 0.0, max_esp_lec = 0.0;
    for (int i = 0; i < L; ++i) {
        suma_esp_lec += metricas[i].suma_espera_us;
        if (metricas[i].max_espera_us > max_esp_lec) max_esp_lec = metricas[i].max_espera_us;
    }

    double suma_esp_esc = 0.0, max_esp_esc = 0.0;
    for (int j = 0; j < E; ++j) {
        int id = static_cast<int>(L) + j;
        suma_esp_esc += metricas[id].suma_espera_us;
        if (metricas[id].max_espera_us > max_esp_esc) max_esp_esc = metricas[id].max_espera_us;
    }

    double prom_esp_lec_us = suma_esp_lec / (L * K);
    double prom_esp_esc_us = suma_esp_esc / (E * K);

    long total_ops = (L + E) * K;
    long esp_version = E * K;
    long esp_doble = 2 * E * K;

    bool correcto = (reg.version == esp_version) &&
                    (reg.doble_version == esp_doble) &&
                    (diag_inconsistencias.load() == 0) &&
                    (!diag_violacion_exclusion.load());

    // 8. Imprimir resumen de ejecución (Sección 5)
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "=== Resumen de Ejecucion ===\n";
    std::cout << "Modo: " << modo << " | Politica: " << pol_str << "\n";
    std::cout << "Operaciones totales: " << total_ops << " (L=" << L << ", E=" << E << ", K=" << K << ")\n";
    std::cout << "Registro final: version=" << reg.version << " (esperado " << esp_version << "), "
              << "doble_version=" << reg.doble_version << " (esperado " << esp_doble << ")\n";
    std::cout << "Inconsistencias: " << diag_inconsistencias.load()
              << " | Max lectoras simultaneas: " << diag_max_lectoras.load() << "\n";
    std::cout << "Tiempo pared: " << tiempo_pared_ms << " ms | Tiempo CPU: " << tiempo_cpu_ms << " ms\n";
    std::cout << "Espera lectoras (prom/max): " << prom_esp_lec_us << " us / " << max_esp_lec << " us\n";
    std::cout << "Espera escritoras (prom/max): " << prom_esp_esc_us << " us / " << max_esp_esc << " us\n";
    std::cout << "correcto=" << (correcto ? "SI" : "NO") << "\n";

    //R6
    if (modo == "cond" && con_traza && ptr_monitor != nullptr) {
        auto res = verificar_traza(ptr_monitor->obtener_traza(), politica);
        
        std::cout << "\n--- Verificacion de Traza (R6) ---\n";
        if (!res.politica_ok) {
            std::cerr << "FALLO: La traza NO cumple las reglas de exclusion mutua o politica.\n";
        } else {
            std::cout << "EXITO: La traza es correcta segun la politica.\n";
        }

        std::cout << "politica_ok=" << (res.politica_ok ? "SI" : "NO")
                  << " | indicador_evidencia=" << res.indicador_evidencia << "\n";
    } 
    //fin R6
    else if (modo == "atomic" && ptr_atomico != nullptr) {
        auto stats = estadisticas_totales;
        std::cout << "Intentos fallidos atomic -> ocupado: " << stats.fallos_ocupado
                  << ", escritores_esperando: " << stats.fallos_escritores_esperando
                  << ", cas: " << stats.fallos_cas << "\n";
    }

    return correcto ? 0 : 1;
}