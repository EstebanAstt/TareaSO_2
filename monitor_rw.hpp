#pragma once
#include "control_rw.hpp"
#include <mutex>
#include <condition_variable>

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
    //ACA SE AÑADEN LAS condition_variable Y CONTADORES DE ESTADO
};