include ../config.mk

all: ross_util.o

ross_util.o: ross_util.cpp ross_util.h
	g++ $(OPTS) -c ross_util.cpp -o $@

clean:
	rm -rf *.o
