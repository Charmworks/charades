include ../../src/config.mk

SUFFIX_B = 
EXTRA_OPTS = 
CFLAGS += -w -O3 $(EXTRA_OPTS)
#CFLAGS = -O5 -qstrict -qprefetch=aggressive -qarch=qp -qtune=qp -qmaxmem=-1 -qsimd=noauto -qreport -qhot $(EXTRA_OPTS)
LIBDIR = ../../src/build$(SUFFIX_B)/

all: phold$(SUFFIX_B)
projections: phold.prj
everything : phold phold.prj

clean:
	rm -f phold
	rm -f *.o *~ core*


phold$(SUFFIX_B): phold.C phold.h $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) phold.C -o $@ $(LDFLAGS) -L$(LIBDIR) -lross -module NDMeshStreamer -module completion

phold$(SUFFIX_B).prj: phold.C phold.h $(LIBDIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) phold.C -o $@ $(LDFLAGS) -L$(LIBDIR) -lross -tracemode projections

