#include "../include/control_atomico_rw.hpp"

void ControlAtomicoRW::entrar_lectura() {
    while (true){ // se hace un while ya que la hebra lectora debe leer hasta que pueda entrar (no haya escritora editando o esperando)
        std::uint64_t estado_actual = estado.load(); //se crea una variable con 64 bits
        bool hay_escritora_activa =
            (estado_actual & BIT_ESCRITORA) != 0; //revisa el bit 16 con una mascara para ver si hay escritoras o no

        bool hay_escritoras_esperando = 
            (estado_actual & (0xFFFFULL << 32)) != 0; //revisa los 16 bits en adelante desde el 32 hacia la izquierda, tambien con una mascara si encuentra algo desigual a 0 significa que hay escritoras esperando
        
        if (hay_escritora_activa || hay_escritoras_esperando){
            continue;}// si existe escritora actuva o esperando vuelve al inciio del while
        
        std::uint64_t nuevo_estado = estado_actual + UNA_LECTORA; // se crea una nueva variable con el estado actual pero se le suma una lectora activa
    
        if (estado.compare_exchange_weak(estado_actual, nuevo_estado)) { // si estado actual y nuevo estado coinciden, significa que logro incrementar el contador, ya tiene permiso para leer, si CAS falla vuelve a empezar
            return;
        }
    }
}

void ControlAtomicoRW::salir_lectura() {
    estado.fetch_sub(UNA_LECTORA); 
    // .fetch_sub resta una cantidad a estado de manera atomica, y sabe que la cantidad es de UNA_LECTORA (1ULL)
}

void ControlAtomicoRW::entrar_escritura() {
    estado.fetch_add(UNA_ESPERANDO); //.fetch_add suma una cantidad a estado de manera atomica y sabe que la cantidad es UNA_ESPERANDO (Bit 32)

    while (true) { //se hace un while ya que la hebra escritora debe leer hasta que pueda entrar (no haya escritora o lectoras dentro)
        std::uint64_t estado_actual = estado.load(); 

        bool hay_lectoras_activas = //verefica si existen lectoras activas
            (estado_actual & 0xFFFFULL) != 0;

        bool hay_escritora_activa = //verefica si hay escritoras activas
            (estado_actual & BIT_ESCRITORA) != 0;

        if (hay_lectoras_activas || hay_escritora_activa) { // si hay lectoras o escritora activa vuelve al inicio del while
            continue;
        }
        std::uint64_t nuevo_estado = (estado_actual - UNA_ESPERANDO) | BIT_ESCRITORA; // el nuevo estado sera el estado actual - una escritora esperando y activa el bit de escritora activa

        if (estado.compare_exchange_weak(estado_actual, nuevo_estado)) { // si la comparación es correcta, CAS es exitoso la escritora esta dentro, sino vuelve a hacer el proceso
            return;
        }
    }
}

void ControlAtomicoRW::salir_escritura() {
    estado.fetch_sub(BIT_ESCRITORA); // .fetch_sub resta una cantidad a estado de manera atomica, en este caso por BIT_ESCRITORA estara restando en el bit 16, bit correspondiente a escritora activa
    
}

EstadisticasAtomico ControlAtomicoRW::obtener_estadisticas() const {
    return EstadisticasAtomico{};
}
