include ../../src/config.mk

CFLAGS := -O2 -g
LIBDIR := ../../src/build/

all: dragonfly

dragonfly.o: dragonfly.C $(LIBDIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) dragonfly.C -c -o dragonfly.o

dragonfly: dragonfly.o $(LIBDIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) dragonfly.o -o $@ $(LDFLAGS) -L$(LIBDIR) -lcharades

clean:
	rm -f dragonfly
	rm -f *.o *~ core*
