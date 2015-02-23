include ../../src/config.mk

CFLAGS  := -O2 -g
LIBDIR  := ../../build/

all: pcs

pcs.o: pcs.C $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) pcs.C -c -o pcs.o 

pcs: pcs.o $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) pcs.o -o $@ $(LDFLAGS) -L$(LIBDIR) -lross

clean:
	rm -f pcs
	rm -f *.o *~ core*


