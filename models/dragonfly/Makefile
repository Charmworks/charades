include ../../src/config.mk

all: dragonfly

dragonfly: dragonfly.C dragonfly.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) dragonfly.C -o dragonfly $(LFLAGS) -L$(BUILD_DIR) -lcharades

clean:
	rm -f dragonfly
	rm -f *.o *~ core*
