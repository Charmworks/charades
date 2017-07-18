include ../../src/config.mk

all: example
clean:
	rm -f example charmrun
	rm -f *.o *~ core*


example: example.C example.h $(BUILD_DIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) -module CommonLBs example.C -o example $(LDFLAGS) -L$(BUILD_DIR) -lross
