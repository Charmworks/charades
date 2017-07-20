include ../../src/config.mk

all: example

example: example.C example.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) example.C -o example $(LFLAGS) -L$(BUILD_DIR) -lcharades

clean:
	rm -f example charmrun
	rm -f *.o *~ core*
