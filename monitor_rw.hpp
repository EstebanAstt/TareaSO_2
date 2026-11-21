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

    // Estado de admisión (R3 y R4)
    int lectoras_activas = 0;
    int lectoras_esperando = 0;
    bool escritora_activa = false;
    int escritoras_esperando = 0;

    std::condition_variable cv_lectoras;
    std::condition_variable cv_escritoras;

    void anotar(Tipo tipo, Accion accion, long turno);
};