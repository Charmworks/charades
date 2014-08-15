include ../config.mk

all: pe.o lp.o

ci_files: ross.ci pe.ci lp.ci event.ci
	$(CHARMC) ross.ci pe.ci lp.ci event.ci

pe.o: ci_files pe.h pe.C
	$(CHARMC) $(OPTS) -c pe.C -o $@

lp.o: ci_files lp.h lp.C
	$(CHARMC) $(OPTS) -c lp.C -o $@

event.o: ci_files event.h event.C
	$(CHARMC) $(OPTS) -c event.C -o $@

clean:
	rm -rf *.o *.decl.h *.def.h
