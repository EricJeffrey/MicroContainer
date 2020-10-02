
.PHONY: all starter attacher clean

all: clean starter attacher

starter: starter.cpp
	g++ -g -Wall -lpthread -o starter.out starter.cpp
attacher: attacher.cpp
	g++ -g -Wall -lpthread -o attacher.out attacher.cpp

clean:
	rm -f attach foo.out starter.out attacher.out
	-crun delete hello