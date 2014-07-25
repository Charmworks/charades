OPTS=

# Turn on this option to test if code compiles without globals
#OPTS+=-DNO_GLOBALS

# Turn on this option to test if code compiles without external forward decls
#OPTS+=-DNO_FORWARD_DECL

all: ross_setup.o

ross_setup.o: ross_setup.cpp ross_setup.h
	g++ $(OPTS) -c ross_setup.cpp -o $@

clean:
	rm -rf *.o
