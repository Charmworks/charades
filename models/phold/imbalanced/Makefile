include ../../../src/config.mk

ifneq ($(USE_CHARMC),1)
include ../../../src/charmc_flags.mk
CC=$(CHARM_CXX)
CFLAGS += $(CHARM_CXX_FLAGS)
LFLAGS += $(CHARM_LDXX_FLAGS)
else
CC=$(CHARMC)
endif


all: phold
clean:
	rm -f phold charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv

phold: phold.C phold.h $(BUILD_DIR)/libcharades.a
	$(CC) phold.C -o phold -L$(BUILD_DIR) -lcharades $(CFLAGS) $(LFLAGS)
