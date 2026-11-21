cmake_minimum_required(VERSION 4.3)
project(TareaSO_2)

set(CMAKE_CXX_STANDARD 17)

add_executable(TareaSO_2
        control_rw.hpp
        monitor_rw.hpp
        control_atomico_rw.hpp
        verificador.hpp
        monitor_rw.cpp
        control_atomico_rw.cpp
        rw.cpp
        verificador.cpp)
