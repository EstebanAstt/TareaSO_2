#pragma once
#include <vector>
#include <cstdint>

enum class Politica { Lectores, Escritores, Fifo };
enum class Tipo { Lectora, Escritora };
enum class Accion { Pide, Entra, Sale };

struct AnotacionTraza {
    int hebra;      // identificador lógico de la hebra
    Tipo tipo;
    Accion accion;
    long turno;     // turno FIFO; -1 en las otras políticas
};

struct Registro {
    long version = 0;
    long doble_version = 0;
};

// Variable global por hebra: cada thread tiene su propia copia independiente
extern thread_local int id_hebra;

class ControlRW {
public:
    virtual ~ControlRW() = default;
    virtual void entrar_lectura() = 0;
    virtual void salir_lectura() = 0;
    virtual void entrar_escritura() = 0;
    virtual void salir_escritura() = 0;
};