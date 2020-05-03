all: test
	./test >result.log
	cat result.log

test: test.cpp ../m6502.hpp
	clang++ -o test test.cpp
