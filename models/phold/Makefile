include ../../src/config.mk

CFLAGS  := -w -O3
LIBDIR  := ../../src/build/
#ROSS=-I/dcsdata/home/mikida2/ross/src/

all: phold
clean:
	rm -f phold
	rm -f *.o *~ core*


phold: phold.C $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) phold.C -o phold $(LDFLAGS) -L$(LIBDIR) -lross
