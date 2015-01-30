include ../../src/config.mk

CFLAGS  := -g -Wall
#CHARMC = ~/charm/bin/charmc
LIBDIR  := ../../src/build/
#ROSS=-I/dcsdata/home/mikida2/ross/src/

all: pcs.o pcs
clean:
	rm -f pcs
	rm -f *.o *~ core*

pcs.o: pcs.C $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) pcs.C -o pcs.o $(LDFLAGS) -L$(LIBDIR) -lross

pcs: pcs.C $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) pcs.C -o pcs $(LDFLAGS) -L$(LIBDIR) -lross
