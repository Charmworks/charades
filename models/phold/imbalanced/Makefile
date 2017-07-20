include ../../../src/config.mk

all: phold

phold: phold.C phold.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) phold.C -o phold $(LFLAGS) -L$(BUILD_DIR) -lcharades

clean:
	rm -f phold charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv
