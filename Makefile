all:1730sh test

1730sh:1730sh.o
	g++ -o 1730sh 1730sh.o
1730sh.o:1730sh.cpp
	g++ -Wall -std=c++14 -g -O0 -pedantic-errors -c 1730sh.cpp

test:test.o
	g++ -o test test.o
test.o:test.cpp
	g++ -c test.cpp

clean:
	rm -f 1730sh
	rm -f 1730sh.o	
	rm -f test
	rm -f test.o