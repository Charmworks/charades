#CHARMC=~/Programming/libraries/charm/bin/charmc
CHARMC=~/git/charm/bin/charmc

all: lp

ross: ross.ci
	$(CHARMC) ross.ci

lp_generated: lp.ci
	$(CHARMC) -c lp.ci

pe_generated: pe.ci
	$(CHARMC) -c pe.ci

pe: pe_queue pe_generated pe.h pe.C
	$(CHARMC) -c pe.C -o pe.o

lp: lp_generated lp.h lp.C pe_generated pe.h event
	$(CHARMC) -c lp.C -o lp.o

pe_queue: lp_generated pe_queue.h pe_queue.cpp
	$(CHARMC) -c pe_queue.cpp -o pe_queue.o

event: lp_generated event.cpp event.h
	$(CHARMC) -c event.cpp -o event.o

clean:
	rm -rf *.o *.decl.h *.def.h
