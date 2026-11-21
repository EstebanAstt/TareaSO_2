#!/bin/bash

EXEC="./build/TareaSO_2" #ajusta la ruta a tu ejecutable generado por CMake

echo "======================================================"
echo " EXPERIMENTOS E1: Corrección y prioridades"
echo "======================================================"
#configuraciones: L E K dur_lec dur_esc
configs=(
    "1 1 20 500 500"
    "2 6 100 200 500"
    "6 2 100 500 200"
    "8 8 100 0 0"
    "1 8 200 1000 100"
)

for config in "${configs[@]}"; do
    echo "------------------------------------------------------"
    echo ">>> Probando configuración: $config"
    
    echo "[Monitor - Política Lectores]"
    $EXEC cond lectores $config
    
    echo "[Monitor - Política Escritores]"
    $EXEC cond escritores $config
    
done

echo ""
echo "======================================================"
echo " EXPERIMENTOS E2: Costo de sincronización y análisis"
echo "======================================================"
#configuracin E2: L=8, E=8, K=10000, dur_lec=0, dur_esc=0, notraza
config_e2="8 8 10000 0 0 notraza"

echo ">>> Ejecutando estrés (K=10000) sin traza:"
echo "[Monitor - Política Escritores]"
$EXEC cond escritores $config_e2
