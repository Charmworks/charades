all: ross_rand

ross_rand: ross-random.h ross-random.c rand-clcg4.h rand-clcg4.c
	g++ -c ross-random.c rand-clcg4.c

clean:
	rm -rf *.o
