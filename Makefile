# Paths
MAIN_PATH=./core/test/main.cpp
OUT_PATH=./x64/misim

# Option configurations
CXX=g++
CINC=-I./core/
CFLAGS=-fcoroutines -pthread -std=c++23

# Command line
misim: $(MAIN_PATH)
	$(CXX) -o $(OUT_PATH) $(MAIN_PATH) $(CINC) $(CFLAGS)
