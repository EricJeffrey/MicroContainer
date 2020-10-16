.PHONY: test

test: test/test_main.cpp test/*.h *.cpp *.h
	g++ -g -Wall -lleveldb --std=c++17 -o test.out *.cpp test/test_main.cpp