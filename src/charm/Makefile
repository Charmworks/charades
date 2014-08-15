include ../config.mk

all: pe.o lp.o

ci_files: ross.ci pe.ci lp.ci event.ci
	$(CHARMC) ross.ci pe.ci lp.ci event.ci

ross.o: ci_files ross.h ross.C
	$(CHARMC) $(OPTS) -c ross.C -o $@

pe.o: ci_files pe.h pe.C
	$(CHARMC) $(OPTS) -c pe.C -o $@

lp.o: ci_files lp.h lp.C
	$(CHARMC) $(OPTS) -c lp.C -o $@

event.o: ci_files event.h event.C
	$(CHARMC) $(OPTS) -c event.C -o $@

clean:
	rm -rf *.o *.decl.h *.def.h
