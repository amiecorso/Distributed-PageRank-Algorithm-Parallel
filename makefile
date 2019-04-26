GCC=mpicc.mpich

all: part2.c
	$(GCC) ./part2.c -o prog

run: prog
	mpiexec.mpich -np 2 prog

clean:
	rm *.o output.txt prog
