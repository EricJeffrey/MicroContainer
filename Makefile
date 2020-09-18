CCFLAGS=-g -c -Wall -Ilib
LLIB=-lssl -lz -lcrypto -lpthread -larchive
CCMACRO=-D CPPHTTPLIB_OPENSSL_SUPPORT

.PHONY: main clean clean-build

main: main.o logger.o httplib.o pull.o extract.o network.o utils.o
	g++ -g -o microc main.o logger.o httplib.o pull.o extract.o $(CCMACRO) $(LLIB)

main.o: main.cpp pull.h
	g++ $(CCFLAGS) -o main.o main.cpp

logger.o: lib/logger.cpp lib/logger.h
	g++ $(CCFLAGS) -o logger.o lib/logger.cpp

httplib.o: lib/httplib.cc lib/httplib.h
	g++ $(CCFLAGS) $(CCMACRO) -o httplib.o lib/httplib.cc

pull.o: pull.cpp pull.h imagerepo.h
	g++ $(CCFLAGS) $(CCMACRO) -o pull.o pull.cpp

extract.o: extract.cpp extract.h
	g++ $(CCFLAGS) -o extract.o extract.cpp

network.o: network.h network.cpp utils.o
	g++ $(CCFLAGS) -o network.o network.cpp

utils.o: utils.h utils.cpp
	g++ $(CCFLAGS) -o utils.o utils.cpp

clean:
	rm -f *.o *.out

clean-build:
	rm -rf build/images/*
	rm -rf build/layers/*
	rm -rf build/containers/*
	rm -rf build/overlay/*
