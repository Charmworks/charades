#CHARMC=~/Programming/libraries/charm/bin/charmc
CHARMC ?= ~/charm/net-linux-x86_64_debug/bin/charmc
OPTS=

all: libross.a

libross.a: lp.o pe_queue.o event.o ross_opts.o ross_setup.o
	$(CHARMC) -o $@ $^

ross: ross.ci
	$(CHARMC) $<

lp_generated: lp_chare.ci
	$(CHARMC) $<

pe_generated: pe.ci
	$(CHARMC) $<

pe: pe.C pe.h pe_queue pe_generated
	$(CHARMC) $(OPTS) -c pe.C -o pe.o

lp.o:  lp_chare.C lp_chare.h lp_structs.h pe_generated pe.h lp_generated event.o
	$(CHARMC) $(OPTS) -c lp_chare.C -o $@

pe_queue.o: pe_queue.cpp pe_queue.h lp_generated
	$(CHARMC) $(OPTS) -c pe_queue.cpp -o $@

event.o: event.cpp event.h lp_generated
	$(CHARMC) $(OPTS) -c event.cpp -o $@

ross_opts.o: ross_opts.cpp ross_opts.h
	$(CHARMC) $(OPTS) -c ross_opts.cpp -o $@

ross_setup.o: ross_setup.cpp ross_setup.h
	$(CHARMC) $(OPTS) -c ross_setup.cpp -o $@

clean:
	rm -rf *.o *.decl.h *.def.h libross.a
