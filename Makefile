CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread -Iinclude

SRCS = src/rw.cpp src/monitor_rw.cpp src/control_atomico_rw.cpp src/verificador.cpp
OBJS = $(SRCS:.cpp=.o)

all: rw

rw: $(OBJS)
	$(CXX) $(CXXFLAGS) -o rw $(OBJS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) rw