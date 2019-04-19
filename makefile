SRC=part1.c
OBJ=$(SRC:.c=.o)

prog: $(OBJ)
	gcc $(OBJ) -o prog

.C.o: $<
	gcc  -g -I. -c $<

clean:
	rm *.o prog

