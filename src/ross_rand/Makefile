include ../config.mk

all: ross_random.o ross_clcg4.o

ross_random.o: ross_random.cpp ross_random.h
	g++ $(OPTS) -c ross_random.cpp -o $@


ross_clcg4.o: ross_clcg4.cpp ross_clcg4.h
	g++ $(OPTS) -c ross_clcg4.cpp -o $@

clean:
	rm -rf *.o
