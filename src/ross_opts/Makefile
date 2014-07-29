OPTS=

# Turn on this option to test if code compiles without globals
OPTS+=-DNO_GLOBALS

# Turn on this option to test if code compiles without external forward decls
#OPTS+=-DNO_FORWARD_DECL

all: ross_opts.o

ross_opts.o: ross_opts.cpp ross_opts.h
	g++ $(OPTS) -c ross_opts.cpp -o $@

clean:
	rm -rf *.o
