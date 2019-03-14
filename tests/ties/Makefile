include ../../src/config.mk

ifneq ($(USE_CHARMC),1)
include ../../src/charmc_flags.mk
CC=$(CHARM_CXX)
CFLAGS += $(CHARM_CXX_FLAGS)
LFLAGS += $(CHARM_LDXX_FLAGS)
else
CC=$(CHARMC)
endif

all: arithmetic
clean:
	rm -f arithmetic charmrun
	rm -f *.o *~ core*

arithmetic: arithmetic.C arithmetic.h $(BUILD_DIR)/libcharades.a
	$(CC) $(CFLAGS) $(LFLAGS) arithmetic.C -o arithmetic -L$(BUILD_DIR) -lcharades

test: arithmetic
	./charmrun +p1 ++local ./arithmetic
	./charmrun +p2 ++local ./arithmetic
