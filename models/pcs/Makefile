include ../../src/config.mk

all: pcs

pcs: pcs.C pcs.h $(LIBDIR)/libcharades.a
	$(CHARMC) $(CFLAGS) pcs.C -o pcs $(LFLAGS) -L$(BUILD_DIR) -lcharades

clean:
	rm -f pcs
	rm -f *.o *~ core*
