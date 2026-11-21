#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), con_traza(registrar_traza) {}

void MonitorRW::entrar_lectura() {
    // Implementación real pendiente (Parte I)
}

void MonitorRW::salir_lectura() {
    // Implementación real pendiente (Parte I)
}

void MonitorRW::entrar_escritura() {
    // Implementación real pendiente (Parte I)
}

void MonitorRW::salir_escritura() {
    // Implementación real pendiente (Parte I)
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
