#CHARMC=~/Programming/libraries/charm/bin/charmc
CHARMC ?= ~/charm/net-linux-x86_64_debug/bin/charmc
OPTS =

INCS = -Icollections
C_FLAGS=$(INCS) $(OPTS)

all: libross.a

libross.a: lp.o pe_queue.o event.o globals.o
	$(CHARMC) -o $@ $^

ross: ross.ci
	$(CHARMC) $<

lp_generated: lp_chare.ci
	$(CHARMC) $<

pe_generated: pe.ci
	$(CHARMC) $<

pe: pe.C pe.h pe_queue pe_generated
	$(CHARMC) $(C_FLAGS) -c $< -o pe.o

lp.o:  lp_chare.C lp_chare.h lp_structs.h pe_generated pe.h lp_generated event.o
	$(CHARMC) $(C_FLAGS) -c $< -o $@

pe_queue.o: collections/pe_queue.cpp collections/pe_queue.h lp_generated
	$(CHARMC) $(C_FLAGS) -c $< -o $@

event.o: event.cpp event.h lp_generated
	$(CHARMC) $(C_FLAGS) -c event.cpp -o $@

globals.o: globals.cpp globals.h
	$(CHARMC) $(C_FLAGS) -c globals.cpp -o $@

clean:
	rm -rf *.o *.decl.h *.def.h libross.a
