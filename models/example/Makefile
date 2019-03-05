include ../../src/config.mk

ifneq ($(USE_CHARMC),1)
include ../../src/charmc_flags.mk
CC=$(CHARM_CXX)
CFLAGS += $(CHARM_CXX_FLAGS)
LFLAGS += $(CHARM_LDXX_FLAGS)
else
CC=$(CHARMC)
endif

all: example
clean:
	rm -f example charmrun
	rm -f *.o *~ core*

example: example.C example.h $(BUILD_DIR)/libcharades.a
	$(CC) $(CFLAGS) $(LFLAGS) example.C -o example -L$(BUILD_DIR) -lcharades
