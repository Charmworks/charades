CHARMC=~/Programming/libraries/charm/bin/charmc

all: ross pe lp

ross: ross.ci
	$(CHARMC) ross.ci

pe: pe.ci pe_queue.h pe_queue.cpp
	$(CHARMC) -c pe.ci pe_queue.cpp

lp: lp.ci lp.h lp.C
	$(CHARMC) -c lp.ci lp.C -o lp.o

clean:
	rm -rf *.o *.decl.h *.def.h
