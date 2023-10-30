# Paths
MAIN_PATH=./src/main.cpp
OUT_PATH=./misim

# Option configurations
CXX=g++
CINC=-I./inc/
CFLAGS=-fcoroutines -pthread -std=c++23

# Command line
misim: $(MAIN_PATH)
	$(CXX) -o $(OUT_PATH) $(MAIN_PATH) $(CINC) $(CFLAGS)

# Clear
clear:
	rm *.bin
	rm misim
	rm *.log
