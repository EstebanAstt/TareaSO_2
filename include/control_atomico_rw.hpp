#pragma once
#include "control_rw.hpp"
#include <atomic>

struct EstadisticasAtomico {
    long fallos_ocupado = 0;
    long fallos_escritores_esperando = 0;
    long fallos_cas = 0;
};

class ControlAtomicoRW : public ControlRW {
public:
    ControlAtomicoRW() = default;
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

    // Método auxiliar para recoger los contadores locales de cada hebra al final (R10)
    EstadisticasAtomico obtener_estadisticas() const;

private:
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
     // Formato cartel 64 bits (palabra atomica)
    static constexpr std::uint64_t UNA_LECTORA   = 1ULL; //   bits  0-15 : lectoras dentro (lectoras activas)
    static constexpr std::uint64_t BIT_ESCRITORA = 1ULL << 16; //   bit   16   : escritora dentro (0 o 1)
    static constexpr std::uint64_t UNA_ESPERANDO = 1ULL << 32; //   bits 32-47 : escritoras esperando
    std::atomic<std::uint64_t> estado{0};
};