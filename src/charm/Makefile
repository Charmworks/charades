CHARMC = ~/charm/net-linux-x86_64_debug/bin/charmc
OPTS=-I.. -I../collections -I.

# Turn on this option to test if code compiles without globals
#OPTS+=-DNO_GLOBALS

# Turn on this option to test if code compiles without external forward decls
#OPTS+=-DNO_FORWARD_DECL

all: pe.o lp.o

ci_files: ross.ci pe.ci lp.ci
	$(CHARMC) ross.ci pe.ci lp.ci

pe.o: ci_files pe.h pe.C
	$(CHARMC) $(OPTS) -c pe.C -o $@

lp.o: ci_files lp.h lp.C
	$(CHARMC) $(OPTS) -c lp.C -o $@

clean:
	rm -rf *.o *.decl.h *.def.h
