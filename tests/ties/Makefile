include ../../src/config.mk

all: arithmetic
clean:
	rm -f arithmetic charmrun
	rm -f *.o *~ core*


arithmetic: arithmetic.C arithmetic.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) -module CommonLBs arithmetic.C -o arithmetic $(LDFLAGS) -L$(BUILD_DIR) -lcharades
