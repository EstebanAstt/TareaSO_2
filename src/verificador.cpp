#include "../include/verificador.hpp"
#include <iostream>

ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza>& traza, Politica politica) {
    ResultadoVerificacion res{true, 0};

    int lectoras_activas = 0;
    int escritoras_activas = 0;
    int lectoras_pendientes = 0;
    int escritoras_pendientes = 0;

    int idx = 0;

    for (const auto& a : traza) {
        if (a.accion == Accion::Pide) {
            if (a.tipo == Tipo::Lectora) {
                lectoras_pendientes++;
                //evidencia Escritores: lectoras que pidieron mientras habia escritoras pendientes[cite: 12]
                if (politica == Politica::Escritores && escritoras_pendientes > 0) {
                    res.indicador_evidencia++;
                }
            } else {
                escritoras_pendientes++;
            }
        } 
        else if (a.accion == Accion::Entra) {
            if (a.tipo == Tipo::Lectora) {
                lectoras_pendientes--;
                lectoras_activas++;

                //regla TODAS: Una lectora entra sin escritora activa[cite: 11]
                if (escritoras_activas > 0) {
                    res.politica_ok = false;
                    std::cerr << "Error [" << idx << "]: Lectora entra con escritora activa.\n";
                }
                
                //regla ESCRITORES: Una lectora entra sin escritoras pendientes[cite: 11]
                if (politica == Politica::Escritores && escritoras_pendientes > 0) {
                    res.politica_ok = false;
                    std::cerr << "Error [" << idx << "]: Lectora entra habiendo escritoras pendientes.\n";
                }
                
                //evidencia LECTORES: lectoras que entraron mientras había escritoras pendientes[cite: 12]
                if (politica == Politica::Lectores && escritoras_pendientes > 0) {
                    res.indicador_evidencia++;
                }
                
                //evidencia FIFO: lectoras que entraron mientras otra lectora estaba activa[cite: 12]
                if (politica == Politica::Fifo && lectoras_activas > 1) {
                    res.indicador_evidencia++;
                }
                
            } else { //tipo::Escritora
                escritoras_pendientes--;
                escritoras_activas++;

                //regla TODAS:una escritora entra sin lectoras ni escritora activas[cite: 11]
                if (lectoras_activas > 0 || escritoras_activas > 1) {
                    res.politica_ok = false;
                    std::cerr << "Error [" << idx << "]: Escritora entra con recurso ocupado (L:" 
                              << lectoras_activas << ", E:" << escritoras_activas << ").\n";
                }
                
                //regla de los lectores: una escritora entra sin lectoras pendientes[cite: 11]
                if (politica == Politica::Lectores && lectoras_pendientes > 0) {
                    res.politica_ok = false;
                    std::cerr << "Error [" << idx << "]: Escritora entra habiendo lectoras pendientes.\n";
                }
            }
        } 
        else if (a.accion == Accion::Sale) {
            if (a.tipo == Tipo::Lectora) {
                lectoras_activas--;
            } else {
                escritoras_activas--;
            }
        }
        idx++;
    }
    
    return res;
}