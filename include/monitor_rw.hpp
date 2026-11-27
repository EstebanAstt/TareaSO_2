#pragma once
#include "control_rw.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <vector>

class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica p, bool registrar_traza = true);
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;
    std::vector<AnotacionTraza> obtener_traza();

private:
    Politica politica;
    bool con_traza;
    std::mutex m;
    std::vector<AnotacionTraza> traza;

    // Estado de admisión (R3 y R4, privado en MonitorRW): Para controlar tanto R3 como R4, el monitor necesita
    // guardar 4 datos de estado (privados, protegidos siempre por std::mutex m) y 2 salas de espera
    int lectoras_activas = 0;      // Cuántas lectoras están leyendo en este instante
    int lectoras_esperando = 0;    // Cuántas lectoras están dormidas en la fila
    bool escritora_activa = false; // true si hay 1 escritora modificando el registro
    int escritoras_esperando = 0;  // Cuántas escritoras están dormidas en la fila


    //2 variables de condicion separadas (para R7): Si tuviéramos una sola sala de espera mezclada y llamáramos a
    //notify_one(), podríamos despertar por error a una escritora cuando solo pueden pasar lectoras (o viceversa),
    //dejando el programa congelado (deadlock). Al tener cv_lectoras y cv_escritoras por separado, se escoje exactamente
    //a quién despertar.

    std::condition_variable cv_lectoras;
    std::condition_variable cv_escritoras;

    // Estado de admisión exclusivo para R5 (Política FIFO):
    // - siguiente_turno asigna turnos consecutivos desde 0 en orden de llegada al monitor.
    // - turno_actual indica el único turno autorizado a concretar su ingreso (Entra), lo que
    //   garantiza orden estricto FIFO (R5 y R6) y permite admitir lectoras consecutivas en paralelo.
    // - turno_activo guarda el turno de cada hebra activa para registrarlo también en Sale.
    long siguiente_turno = 0;
    long turno_actual = 0;
    std::condition_variable cv_fifo;
    std::unordered_map<std::thread::id, long> turno_activo;

    void anotar(Tipo tipo, Accion accion, long turno);

    // Métodos privados auxiliares para R5 (Política FIFO), invocados siempre con lock(m) adquirido
    void entrar_lectura_fifo(std::unique_lock<std::mutex>& lock);
    void salir_lectura_fifo();
    void entrar_escritura_fifo(std::unique_lock<std::mutex>& lock);
    void salir_escritura_fifo();
};