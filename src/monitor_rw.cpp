#include "../include/monitor_rw.hpp"
//En R3, las lectoras tienen pase libre siempre que no haya una escritora escribiendo en ese momento, incluso si ya hay
//escritoras haciendo fila desde antes. Además, si una escritora termina y hay lectoras y escritoras esperando, las
//lectoras pasan primero.

//En R4 (Prioridad de escritores), la regla central se invierte respecto a R3: apenas hay una escritora escribiendo o
//esperando en la fila, se cierra el paso a cualquier lectora nueva. Las lectoras que ya estaban adentro terminan normal
//y, cuando sale la última, pasa una escritora.


MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), con_traza(registrar_traza) {}

void MonitorRW::anotar(Tipo tipo, Accion accion, long turno) {
    if (con_traza) {
        traza.push_back(AnotacionTraza{id_hebra, tipo, accion, turno});
    }
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);


    // Prioridad lectoras: Adquiere std::unique_lock<std::mutex> lock(m);
    // Espera solo si escritora_activa es true, ignorando por completo escritoras_esperando
    if (politica == Politica::Lectores) {
        anotar(Tipo::Lectora, Accion::Pide, -1);
        while (escritora_activa) {
            lectoras_esperando++;
            cv_lectoras.wait(lock);
            lectoras_esperando--;
        }
        lectoras_activas++;
        anotar(Tipo::Lectora, Accion::Entra, -1);
    }


    //Prioridad escritoras: A diferencia de R3, aca la lectora ademas debe esperar si hay escritoras haciendo fila.
    //
    //Nota: como la escritora incrementa escritoras_esperando antes de dormirse y solo lo decrementa cuando logra
    //despertar y tomar el mutex, ninguna lectora nueva puede colarse mientras una escritora está despertando
    else if (politica == Politica::Escritores) {
        anotar(Tipo::Lectora, Accion::Pide, -1);
        while (escritora_activa || escritoras_esperando > 0) {
            lectoras_esperando++;
            cv_lectoras.wait(lock);
            lectoras_esperando--;
        }
        lectoras_activas++;
        anotar(Tipo::Lectora, Accion::Entra, -1);

    } else {
        // R5 (Política FIFO)
        entrar_lectura_fifo(lock);
    }
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);


    //Prioridad lectoras: Adquiere lock(m) y resta lectoras_activas--. Solo si fue la ultima lectora en salir
    //(lectoras_activas == 0), no hay mas lectoras esperando (lectoras_esperando == 0) y hay al menos una escritora
    //esperando (escritoras_esperando > 0) despiera a una escritora (esto porque se debe priorizar a las lectoras)
    if (politica == Politica::Lectores) {
        lectoras_activas--;
        anotar(Tipo::Lectora, Accion::Sale, -1);
        if (lectoras_activas == 0 && lectoras_esperando == 0 && escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        }
    }


    //Prioridad escritoras: Resta lectoras_activas--. Si fue la última lectora en salir (lectoras_activas == 0) y
    //hay escritoras esperando, despierta a una escritora (aca no importa cuantas lectoras esten esperando, porque las
    //escritoras tienen prioridad)
    else if (politica == Politica::Escritores) {
        lectoras_activas--;
        anotar(Tipo::Lectora, Accion::Sale, -1);
        if (lectoras_activas == 0 && escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        }

    } else {
        // R5 (Política FIFO)
        salir_lectura_fifo();
    }
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);


    //Prioridad lectores: Adquiere lock(m). Espera mientras haya una escritora activa/lectoras leyendo/lentoras en fila
    if (politica == Politica::Lectores) {
        anotar(Tipo::Escritora, Accion::Pide, -1);
        while (escritora_activa || lectoras_activas > 0 || lectoras_esperando > 0) {
            escritoras_esperando++;
            cv_escritoras.wait(lock);
            escritoras_esperando--;
        }
        escritora_activa = true;
        anotar(Tipo::Escritora, Accion::Entra, -1);

    }


    //Prioridad escritoras: Una escritora espera solo si hay otra escritora activa o hay lectoras leyendo en ese
    //momento (se ignoran las lectoras_esperando)
    else if (politica == Politica::Escritores) {
        anotar(Tipo::Escritora, Accion::Pide, -1);
        while (escritora_activa || lectoras_activas > 0) {
            escritoras_esperando++;
            cv_escritoras.wait(lock);
            escritoras_esperando--;
        }
        escritora_activa = true;
        anotar(Tipo::Escritora, Accion::Entra, -1);

    } else {
        // R5 (Política FIFO)
        entrar_escritura_fifo(lock);
    }
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);


    //Prioridad lectoras: Adquiere lock(m) y marca escritora_activa = false. Como las lectoras tienen prioridad debe
    //revisar si hay otras lectoras esperando. Si hay, las despierta a todas con notify_all() para que entren juntas.
    //Si no hay lectoras, entonces despierta a una escritora
    if (politica == Politica::Lectores) {
        escritora_activa = false;
        anotar(Tipo::Escritora, Accion::Sale, -1);
        if (lectoras_esperando > 0) {
            cv_lectoras.notify_all();
        } else if (escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        }

    }


    //Prioridad escritoras: Marca escritora_activa = false. Al decidir a quien despertar, siempre revisa primero
    //escritoras_esperando. Recien cuando no queda ninguna escritora pendiente, despierta a todas las lectoras en fila
    else if (politica == Politica::Escritores) {
        escritora_activa = false;
        anotar(Tipo::Escritora, Accion::Sale, -1);
        if (escritoras_esperando > 0) {
            cv_escritoras.notify_one();
        } else if (lectoras_esperando > 0) {
            cv_lectoras.notify_all();
        }

    } else {
        // R5 (Política FIFO)
        salir_escritura_fifo();
    }
}

void MonitorRW::entrar_lectura_fifo(std::unique_lock<std::mutex>& lock) {
    // Cada solicitud recibe un turno consecutivo (desde 0) al registrarse en el monitor
    const long mi_turno = siguiente_turno++;
    anotar(Tipo::Lectora, Accion::Pide, mi_turno);

    // R5 + R6 + R7: espera mientras haya una escritora activa o no sea su turno de ingreso.
    // Exigir mi_turno == turno_actual impide que una lectora adelante a una escritora con
    // turno anterior pendiente y garantiza que el k-esimo Entra corresponda al turno k.
    cv_fifo.wait(lock, [this, mi_turno]() {
        return !escritora_activa && mi_turno == turno_actual;
    });

    lectoras_activas++;
    turno_actual++; // Habilita de inmediato a la siguiente solicitud en orden FIFO
    if (con_traza) {
        turno_activo[std::this_thread::get_id()] = mi_turno;
    }
    anotar(Tipo::Lectora, Accion::Entra, mi_turno);

    // Si hay solicitudes esperando detrás (p. ej., otra lectora consecutiva con turno == turno_actual),
    // se notifica para que pueda entrar simultáneamente mientras esta lectora lee fuera del monitor.
    if (siguiente_turno > turno_actual) {
        cv_fifo.notify_all();
    }
}

void MonitorRW::salir_lectura_fifo() {
    lectoras_activas--;

    long mi_turno = -1;
    if (con_traza) {
        auto it = turno_activo.find(std::this_thread::get_id());
        if (it != turno_activo.end()) {
            mi_turno = it->second;
            turno_activo.erase(it);
        }
    }
    anotar(Tipo::Lectora, Accion::Sale, mi_turno);

    // Al salir la última lectora activa, si hay solicitudes esperando (p. ej., una escritora
    // esperando en turno_actual), se despierta para que pueda ingresar.
    if (lectoras_activas == 0 && siguiente_turno > turno_actual) {
        cv_fifo.notify_all();
    }
}

void MonitorRW::entrar_escritura_fifo(std::unique_lock<std::mutex>& lock) {
    // Cada solicitud recibe un turno consecutivo (desde 0) al registrarse en el monitor
    const long mi_turno = siguiente_turno++;
    anotar(Tipo::Escritora, Accion::Pide, mi_turno);

    // R5 + R6 + R7: la escritora espera a que sea su turno y a que terminen todas las
    // lectoras admitidas antes de su turno (y no haya otra escritora activa).
    // Mientras turno_actual permanece en mi_turno, bloquea el ingreso de lectoras posteriores.
    cv_fifo.wait(lock, [this, mi_turno]() {
        return !escritora_activa && lectoras_activas == 0 && mi_turno == turno_actual;
    });

    escritora_activa = true;
    turno_actual++; // Deja apuntado el siguiente turno, que seguirá bloqueado mientras escritora_activa sea true
    if (con_traza) {
        turno_activo[std::this_thread::get_id()] = mi_turno;
    }
    anotar(Tipo::Escritora, Accion::Entra, mi_turno);
}

void MonitorRW::salir_escritura_fifo() {
    escritora_activa = false;

    long mi_turno = -1;
    if (con_traza) {
        auto it = turno_activo.find(std::this_thread::get_id());
        if (it != turno_activo.end()) {
            mi_turno = it->second;
            turno_activo.erase(it);
        }
    }
    anotar(Tipo::Escritora, Accion::Sale, mi_turno);

    // Al liberar el recurso exclusivo, se notifica para que ingrese la solicitud en turno_actual
    if (siguiente_turno > turno_actual) {
        cv_fifo.notify_all();
    }
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
