all: test
	./test >result.log
	cat result.log

test: test.cpp ../m6502.hpp
	clang++ -std=c++14 -o test test.cpp
