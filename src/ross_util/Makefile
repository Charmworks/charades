OPTS=

# Turn on this option to test if code compiles without globals
OPTS+=-DNO_GLOBALS

# Turn on this option to test if code compiles without external forward decls
#OPTS+=-DNO_FORWARD_DECL

all: ross_util.o

ross_util.o: ross_util.cpp ross_util.h
	g++ $(OPTS) -c ross_util.cpp -o $@

clean:
	rm -rf *.o
