CXXFLAGS=-g -Wall -Ilib -D CPPHTTPLIB_OPENSSL_SUPPORT
LLIB=-lssl -lz -lcrypto -lpthread -larchive

OBJS=container_repo.o create.o extract.o main.o network.o pull.o utils.o httplib.o logger.o
TARGET=microc

.PHONY: main clean clean-build

main: $(OBJS)
	g++ $(CXXFLAGS) -o $(TARGET) $(LLIB) $(CCMACRO) $(OBJS) --std=c++11

container_repo.o: container_repo.cpp container_repo.h lib/logger.h lib/json.hpp

create.o: create.cpp network.h utils.h lib/logger.h create.h config.h container_repo.h lib/json.hpp image_repo.h

extract.o: extract.cpp extract.h

main.o: main.cpp pull.h lib/logger.h lib/httplib.h lib/json.hpp config.h utils.h create.h app.h

network.o: network.cpp network.h utils.h lib/logger.h

pull.o: pull.cpp pull.h lib/logger.h lib/httplib.h lib/json.hpp config.h utils.h extract.h image_repo.h

utils.o: utils.cpp utils.h lib/logger.h

logger.o: lib/logger.cpp lib/logger.h
	g++ $(CXXFLAGS)  -c -o $@ lib/logger.cpp

httplib.o: lib/httplib.cc lib/httplib.h
	g++ $(CXXFLAGS) $(CCMACRO)  -c -o $@ lib/httplib.cc

clean:
	rm -f *.o *.out

clean-build:
	rm -rf build/images/*
	rm -rf build/layers/*
	rm -rf build/containers/*
	rm -rf build/overlay/*
