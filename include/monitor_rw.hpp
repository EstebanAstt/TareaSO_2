#pragma once
#include "control_rw.hpp"
#include <mutex>
#include <condition_variable>
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

    void anotar(Tipo tipo, Accion accion, long turno);
};