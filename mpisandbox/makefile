GCC=mpicxx.mpich

all: main.cpp
	$(GCC) ./main.cpp -o main

run: main
	mpiexec.mpich -np 2 main
