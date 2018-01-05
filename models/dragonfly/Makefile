include ../../src/config.mk

CFLAGS += -O3

all: dragonfly

dragonfly: dragonfly.C $(BUILD_DIR)/libross.a
	$(CHARMC) $(CFLAGS) dragonfly.C -o dragonfly $(LFLAGS) -L$(BUILD_DIR) -lross

clean:
	rm -f dragonfly
	rm -f *.o *~ core*
