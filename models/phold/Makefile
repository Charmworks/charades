include ../../src/config.mk

CFLAGS  := -g -w
LIBDIR  := ../../src/build/
#ROSS=-I/dcsdata/home/mikida2/ross/src/

all: phold
clean:
	rm -f phold
	rm -f *.o *~ core*


phold: phold.C $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) phold.C -o phold $(LDFLAGS) -L$(LIBDIR) -lross
