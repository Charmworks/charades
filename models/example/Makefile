include ../../src/config.mk

all: example
clean:
	rm -f example charmrun
	rm -f *.o *~ core*


example: example.C example.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) -module CommonLBs example.C -o example $(LFLAGS) -L$(BUILD_DIR) -lcharades
