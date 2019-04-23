include ../../src/config.mk

ifneq ($(USE_CHARMC),1)
include ../../src/charmc_flags.mk
CC=$(CHARM_CXX)
CHARMRUN=$(CHARMBIN)/charmrun
CFLAGS += $(CHARM_CXX_FLAGS)
LFLAGS += $(CHARM_LDXX_FLAGS)
else
CC=$(CHARMC)
CHARMRUN=./charmrun
endif

all: arithmetic
clean:
	rm -f arithmetic charmrun
	rm -f *.o *~ core*

arithmetic: arithmetic.C arithmetic.h $(BUILD_DIR)/libcharades.a
	$(CC) $(CFLAGS) $(LFLAGS) arithmetic.C -o arithmetic -L$(BUILD_DIR) -lcharades

test: arithmetic
	$(CHARMRUN) +p1 ++local ./arithmetic
	$(CHARMRUN) +p2 ++local ./arithmetic
