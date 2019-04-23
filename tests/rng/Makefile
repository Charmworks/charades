include ../../src/config.mk

ifneq ($(USE_CHARMC),1)
include ../../src/charmc_flags.mk
CC=$(CHARM_CXX)
CFLAGS += $(CHARM_CXX_FLAGS)
LFLAGS += $(CHARM_LDXX_FLAGS)
else
CC=$(CHARMC)
endif

all: rng
clean:
	rm -f rng charmrun
	rm -f *.o *~ core*

rng: rng.C rng.h $(BUILD_DIR)/libcharades.a
	$(CC) rng.C -o rng -L$(BUILD_DIR) -lcharades $(CFLAGS) $(LFLAGS)

test: rng
	./rng
