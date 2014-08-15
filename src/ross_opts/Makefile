include ../config.mk

all: ross_opts.o

ross_opts.o: ross_opts.cpp ross_opts.h
	g++ $(OPTS) -c ross_opts.cpp -o $@

clean:
	rm -rf *.o
