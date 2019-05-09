GCC=mpicc.mpich

all: part2.c
	$(GCC) ./part2.c -o prog

run: prog
	mpiexec.mpich -np 2 prog /cs/classes/www/16S/cis630/PROJ/fl_compact.tab /cs/classes/www/16S/cis630/PROJ/fl_compact_part.2 3 2

clean:
	rm *.o *output.txt prog
