CXXFLAGS=-g -Wall -D CPPHTTPLIB_OPENSSL_SUPPORT --std=c++17
LLIB=-lssl -lz -lcrypto -lpthread -larchive -lleveldb

OBJS=attach.o cleanup.o container_ls.o container_repo.o container_repo_item.o\
 container_rm.o create.o extract.o image_build.o image_ls.o image_repo.o image_repo_item.o\
 image_rm.o network.o pull.o repo.o run.o start.o stop.o utils.o logger.o httplib.o
TARGET=microc
TESTTARGET=test.out

.PHONY: main clean clean-build test

main: main.o $(OBJS)
	g++ $(CXXFLAGS) -o $(TARGET) main.o $(OBJS) $(LLIB)


attach.o: attach.cpp cleanup.h config.h lib/json.hpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 lib/logger.h

cleanup.o: cleanup.cpp cleanup.h config.h lib/json.hpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 lib/logger.h network.h stop.h

container_ls.o: container_ls.cpp container_ls.h config.h lib/json.hpp \
 container_repo.h container_repo_item.h repo_item.h utils.h sys_error.h \
 repo.h db_error.h lib/logger.h

container_repo.o: container_repo.cpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 config.h lib/json.hpp

container_repo_item.o: container_repo_item.cpp container_repo_item.h \
 repo_item.h utils.h sys_error.h config.h lib/json.hpp

container_rm.o: container_rm.cpp container_rm.h config.h lib/json.hpp \
 container_repo.h container_repo_item.h repo_item.h utils.h sys_error.h \
 repo.h db_error.h image_repo.h image_repo_item.h lib/logger.h

create.o: create.cpp create.h config.h lib/json.hpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 image_repo.h image_repo_item.h lib/logger.h network.h

extract.o: extract.cpp extract.h

image_build.o: image_build.cpp image_build.h image_repo.h config.h \
 lib/json.hpp image_repo_item.h repo_item.h repo.h db_error.h \
 lib/logger.h utils.h sys_error.h

image_ls.o: image_ls.cpp image_repo.h config.h lib/json.hpp \
 image_repo_item.h repo_item.h repo.h db_error.h lib/logger.h

image_repo.o: image_repo.cpp image_repo.h config.h lib/json.hpp \
 image_repo_item.h repo_item.h repo.h db_error.h utils.h sys_error.h

image_repo_item.o: image_repo_item.cpp image_repo_item.h repo_item.h \
 config.h lib/json.hpp utils.h sys_error.h

image_rm.o: image_rm.cpp config.h lib/json.hpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 image_repo.h image_repo_item.h lib/logger.h

main.o: main.cpp attach.h cleanup.h container_ls.h container_rm.h \
 create.h image_build.h image_ls.h image_rm.h lib/CLI11.hpp network.h \
 repo.h db_error.h repo_item.h pull.h config.h lib/json.hpp lib/httplib.h \
 lib/logger.h utils.h sys_error.h run.h start.h stop.h

network.o: network.cpp config.h lib/json.hpp lib/logger.h network.h \
 repo.h db_error.h repo_item.h utils.h sys_error.h

pull.o: pull.cpp pull.h config.h lib/json.hpp lib/httplib.h lib/logger.h \
 utils.h sys_error.h extract.h image_repo.h image_repo_item.h repo_item.h \
 repo.h db_error.h

repo.o: repo.cpp repo.h db_error.h repo_item.h

run.o: run.cpp attach.h create.h image_repo.h config.h lib/json.hpp \
 image_repo_item.h repo_item.h repo.h db_error.h lib/logger.h start.h \
 utils.h sys_error.h

start.o: start.cpp config.h lib/json.hpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 lib/logger.h network.h

stop.o: stop.cpp stop.h cleanup.h config.h lib/json.hpp container_repo.h \
 container_repo_item.h repo_item.h utils.h sys_error.h repo.h db_error.h \
 lib/logger.h

utils.o: utils.cpp utils.h sys_error.h lib/logger.h


logger.o: lib/logger.cpp lib/logger.h
	g++ $(CXXFLAGS)  -c -o $@ lib/logger.cpp

httplib.o: lib/httplib.cc lib/httplib.h
	g++ $(CXXFLAGS)  -c -o $@ lib/httplib.cc

test_main.o: test/test_main.cpp test/test_image_repo.h test/test_utils.h \
 image_repo.h config.h lib/json.hpp image_repo_item.h repo_item.h repo.h \
 db_error.h image_repo_item.h utils.h sys_error.h test/test_container_repo.h \
 config.h container_repo.h container_repo_item.h utils.h test/test_cont_net.h \
 network.h lib/logger.h
	g++ $(CXXFLAGS) -c -o $@ $<


clean:
	rm -f *.o *.out
	rm -f microc

clean-build:
	rm -rf build/*
