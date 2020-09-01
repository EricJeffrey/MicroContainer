
.PHONY: main clean

main: main.o logger.o httplib.o
	g++ -g -o main.out -D CPPHTTPLIB_OPENSSL_SUPPORT -lssl -lz -lcrypto -lpthread main.o logger.o httplib.o

main.o: main.cpp pull.h
	g++ -g -c -Wall -Ilib -o main.o main.cpp

logger.o: lib/logger.cpp lib/logger.h
	g++ -g -c -Wall -Ilib -o logger.o lib/logger.cpp

httplib.o: lib/httplib.cc lib/httplib.h
	g++ -g -c -Wall -D CPPHTTPLIB_OPENSSL_SUPPORT -Ilib -o httplib.o lib/httplib.cc

clean:
	rm -f *.o *.out