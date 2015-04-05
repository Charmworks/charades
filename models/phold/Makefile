include ../../src/config.mk

CFLAGS  := -w -O3

MPI_ROSS_BUILD_DIR := /dcsdata/home/mikida2/ross_mpi/build/core
MPI_ROSS_DIR := /dcsdata/home/mikida2/ross_mpi/core
MPICXX := /usr/bin/mpicxx

all: phold_charm phold_mpi
clean:
	rm -f phold_charm charmrun
	rm -f phold_mpi
	rm -f *.o *~ core*
	rm -f output ross.csv

phold_charm: phold_charm.C phold_charm.h phold_common.h $(BUILD_DIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) -module RotateLB phold_charm.C -o phold_charm $(LDFLAGS) -L$(BUILD_DIR) -lross

phold_mpi: phold_mpi.C phold_mpi.h phold_common.h $(MPI_ROSS_BUILD_DIR)/libROSS.a
	$(MPICXX) $(CFLAGS) phold_mpi.C -o phold_mpi $(LDFLAGS) -I$(MPI_ROSS_DIR) -I$(MPI_ROSS_BUILD_DIR) -L$(MPI_ROSS_BUILD_DIR) -lROSS -lmpi
