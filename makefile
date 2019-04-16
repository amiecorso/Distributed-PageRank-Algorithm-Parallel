SRC=part1.cpp
OBJ=$(SRC:.cpp=.o)

prog: $(OBJ)
	g++ $(OBJ) -o prog

.C.o: $<
	g++  -g -I. -c $<

clean:
	rm *.o prog

