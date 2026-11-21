cmake_minimum_required(VERSION 4.3)
project(TareaSO_2)

set(CMAKE_CXX_STANDARD 17)

add_executable(TareaSO_2
        include/control_rw.hpp
        include/monitor_rw.hpp
        include/control_atomico_rw.hpp
        include/verificador.hpp
        src/monitor_rw.cpp
        src/control_atomico_rw.cpp
        src/rw.cpp
        src/verificador.cpp)
