CHARMC=~/Programming/libraries/charm/bin/charmc

all: ross pe lp

ross: ross.ci
	$(CHARMC) ross.ci

pe: pe.ci
	$(CHARMC) pe.ci

lp: lp.ci lp.h lp.C
	$(CHARMC) -c lp.ci lp.C -o lp.o

clean:
	rm -rf *.o *.decl.h *.def.h
