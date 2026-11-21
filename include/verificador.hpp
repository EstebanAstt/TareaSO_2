#pragma once
#include "control_rw.hpp"

struct ResultadoVerificacion {
    bool politica_ok = true;
    long indicador_evidencia = 0;
};

ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza>& traza, Politica politica);