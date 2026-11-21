cmake_minimum_required(VERSION 3.10) #me dicen que 4.3 no es valido
project(TareaSO_2)

set(CMAKE_CXX_STANDARD 17)

#flags requeridas por la pauta y para ThreadSanitizer
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread -fsanitize=thread -g -O2")

add_executable(TareaSO_2
        include/control_rw.hpp
        include/monitor_rw.hpp
        include/control_atomico_rw.hpp
        include/verificador.hpp
        src/monitor_rw.cpp
        src/control_atomico_rw.cpp
        src/rw.cpp
        src/verificador.cpp)
