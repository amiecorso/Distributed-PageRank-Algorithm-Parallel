GCC=mpicc.mpich

all: part2.c
	$(GCC) -Ofast ./part2.c -o prog

run: prog
	mpiexec.mpich -np 4 prog /cs/classes/www/16S/cis630/PROJ/fl_compact.tab /cs/classes/www/16S/cis630/PROJ/fl_compact_part.4 5 4

clean:
	rm *.o *.out prog
