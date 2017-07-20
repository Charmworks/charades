include ../../../src/config.mk

all: phold
clean:
	rm -f phold charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv


phold: phold.C phold.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $(LFLAGS) phold.C -o phold -L$(BUILD_DIR) -lcharades
