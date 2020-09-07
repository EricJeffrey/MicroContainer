
.PHONY: main clean clean-build

main: main.o logger.o httplib.o pull.o extract.o
	g++ -g -o main.out main.o logger.o httplib.o pull.o extract.o -D CPPHTTPLIB_OPENSSL_SUPPORT -lssl -lz -lcrypto -lpthread -larchive

main.o: main.cpp pull.h
	g++ -g -c -Wall -Ilib -o main.o main.cpp

logger.o: lib/logger.cpp lib/logger.h
	g++ -g -c -Wall -Ilib -o logger.o lib/logger.cpp

httplib.o: lib/httplib.cc lib/httplib.h
	g++ -g -c -Wall -D CPPHTTPLIB_OPENSSL_SUPPORT -Ilib -o httplib.o lib/httplib.cc

pull.o: pull.cpp pull.h
	g++ -g -c -Wall -D CPPHTTPLIB_OPENSSL_SUPPORT -Ilib -o pull.o pull.cpp

extract.o: extract.cpp extract.h
	g++ -g -c -Wall -Ilib -o extract.o extract.cpp

clean:
	rm -f *.o *.out
	rm -rf build/images/*
	rm -rf build/layers/*

clean-build:
	rm -rf build/images/*
	rm -rf build/layers/*
	rm -rf build/containers/*
	rm -rf build/overlay/*
