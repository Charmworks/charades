include ../../src/config.mk

LIBDIR := ../../src/build/

all: dragonfly

dragonfly.o: dragonfly.C $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) dragonfly.C -c -o dragonfly.o

dragonfly: dragonfly.o $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) dragonfly.o -o $@ $(LDFLAGS) -L$(LIBDIR) -lross

clean:
	rm -f dragonfly
	rm -f *.o *~ core*
