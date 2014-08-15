include ../config.mk

all: ross_setup.o

ross_setup.o: ross_setup.cpp ross_setup.h
	$(CC) $(OPTS) -c ross_setup.cpp -o $@

clean:
	rm -rf *.o
