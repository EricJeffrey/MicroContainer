CXXFLAGS=-g -Wall -Ilib -D CPPHTTPLIB_OPENSSL_SUPPORT --std=c++17
LLIB=-lssl -lz -lcrypto -lpthread -larchive -lleveldb

OBJS=image_ls.o container_ls.o container_repo.o container_repo_item.o create.o extract.o\
  image_repo.o image_repo_item.o network.o pull.o repo.o utils.o logger.o httplib.o
TARGET=microc
TESTTARGET=test.out

.PHONY: main clean clean-build test

main: main.o $(OBJS)
	g++ $(CXXFLAGS) -o $(TARGET) $(LLIB) main.o $(OBJS)

test: test_main.o $(OBJS)
	g++ $(CXXFLAGS) -o $(TESTTARGET) $(LLIB) test_main.o $(OBJS)

container_ls.o: container_ls.cpp container_ls.h config.h lib/json.hpp \
 container_repo.h container_repo_item.h repo_item.h utils.h sys_error.h \
 repo.h db_error.h lib/logger.h

container_repo.o: container_repo.cpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h

container_repo_item.o: container_repo_item.cpp container_repo_item.h \
 repo_item.h utils.h sys_error.h

create.o: create.cpp create.h config.h lib/json.hpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 image_repo.h image_repo_item.h lib/logger.h network.h

extract.o: extract.cpp extract.h

image_ls.o: image_ls.cpp image_repo.h config.h lib/json.hpp \
 image_repo_item.h repo_item.h repo.h db_error.h lib/logger.h

image_repo.o: image_repo.cpp image_repo.h config.h lib/json.hpp \
 image_repo_item.h repo_item.h repo.h db_error.h utils.h sys_error.h

image_repo_item.o: image_repo_item.cpp image_repo_item.h repo_item.h \
 utils.h sys_error.h

main.o: main.cpp container_ls.h create.h image_ls.h lib/CLI11.hpp pull.h \
 config.h lib/json.hpp lib/httplib.h lib/logger.h utils.h sys_error.h

network.o: network.cpp network.h utils.h sys_error.h lib/logger.h

pull.o: pull.cpp pull.h config.h lib/json.hpp lib/httplib.h lib/logger.h \
 utils.h sys_error.h extract.h image_repo.h image_repo_item.h repo_item.h \
 repo.h db_error.h

repo.o: repo.cpp repo.h db_error.h repo_item.h

utils.o: utils.cpp utils.h sys_error.h lib/logger.h

logger.o: lib/logger.cpp lib/logger.h
	g++ $(CXXFLAGS)  -c -o $@ lib/logger.cpp

httplib.o: lib/httplib.cc lib/httplib.h
	g++ $(CXXFLAGS)  -c -o $@ lib/httplib.cc

test_main.o: test/test_main.cpp test/test_image_repo.h test/test_utils.h \
  test/../image_repo.h test/../config.h test/../lib/json.hpp \
  test/../image_repo_item.h test/../repo_item.h test/../repo.h \
  test/../db_error.h test/../image_repo_item.h test/../utils.h \
  test/../sys_error.h test/test_container_repo.h test/../config.h \
  test/../container_repo.h test/../container_repo_item.h test/../utils.h
	g++ $(CXXFLAGS) -c -o $@ $<


clean:
	rm -f *.o *.out
	rm -f microc

clean-build:
	rm -rf build/*
