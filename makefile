SRC=part2.c
OBJ=$(SRC:.c=.o)

prog: $(OBJ)
	gcc $(OBJ) -Ofast -march=[cpu-type] -o prog

.C.o: $<
	gcc  -g -I. -c $<

clean:
	rm *.o prog

