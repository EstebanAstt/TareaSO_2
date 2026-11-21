#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), con_traza(registrar_traza) {}

void MonitorRW::anotar(Tipo tipo, Accion accion, long turno) {
    if (con_traza) {
        traza.push_back(AnotacionTraza{id_hebra, tipo, accion, turno});
    }
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);

    if (politica == Politica::Lectores) {
        anotar(Tipo::Lectora, Accion::Pide, -1);
        while (escritora_activa) {
            lectoras_esperando++;
            cv_lectoras.wait(lock);
            lectoras_esperando--;
        }
        lectoras_activas++;
        anotar(Tipo::Lectora, Accion::Entra, -1);

    } else if (politica == Politica::Escritores) {
        anotar(Tipo::Lectora, Accion::Pide, -1);
        while (escritora_activa || escritoras_esperando > 0) {
            lectoras_esperando++;
            cv_lectoras.wait(lock);
            lectoras_esperando--;
        }
        lectoras_activas++;
        anotar(Tipo::Lectora, Accion::Entra, -1);

    } else {
        // Espacio reservado para R5 (Política FIFO)
    }
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);

    if (politica == Politica::Lectores) {
        lectoras_activas--;
        anotar(Tipo::Lectora, Accion::Sale, -1);
        if (lectoras_activas == 0 && lectoras_esperando == 0 && escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        }

    } else if (politica == Politica::Escritores) {
        lectoras_activas--;
        anotar(Tipo::Lectora, Accion::Sale, -1);
        if (lectoras_activas == 0 && escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        }

    } else {
        // Espacio reservado para R5 (Política FIFO)
    }
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);

    if (politica == Politica::Lectores) {
        anotar(Tipo::Escritora, Accion::Pide, -1);
        while (escritora_activa || lectoras_activas > 0 || lectoras_esperando > 0) {
            escritoras_esperando++;
            cv_escritoras.wait(lock);
            escritoras_esperando--;
        }
        escritora_activa = true;
        anotar(Tipo::Escritora, Accion::Entra, -1);

    } else if (politica == Politica::Escritores) {
        anotar(Tipo::Escritora, Accion::Pide, -1);
        while (escritora_activa || lectoras_activas > 0) {
            escritoras_esperando++;
            cv_escritoras.wait(lock);
            escritoras_esperando--;
        }
        escritora_activa = true;
        anotar(Tipo::Escritora, Accion::Entra, -1);

    } else {
        // Espacio reservado para R5 (Política FIFO)
    }
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);

    if (politica == Politica::Lectores) {
        escritora_activa = false;
        anotar(Tipo::Escritora, Accion::Sale, -1);
        if (lectoras_esperando > 0) {
            cv_lectoras.notify_all();
        } else if (escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        }

    } else if (politica == Politica::Escritores) {
        escritora_activa = false;
        anotar(Tipo::Escritora, Accion::Sale, -1);
        if (escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        } else if (lectoras_esperando > 0) {
            cv_lectoras.notify_all();
        }

    } else {
        // Espacio reservado para R5 (Política FIFO)
    }
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
