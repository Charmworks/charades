include ../../src/config.mk

CFLAGS  := -w -O3
LIBDIR  := ../../build/

all: phold
clean:
	rm -f phold charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv


phold: phold.C phold.h $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) phold.C -o phold $(LDFLAGS) -L$(LIBDIR) -lross
