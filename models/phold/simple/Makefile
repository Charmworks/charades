include ../../../src/config.mk

all: phold
clean:
	rm -f phold charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv


phold: phold.C phold.h $(BUILD_DIR)/libross.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) -module CommonLBs phold.C -o phold $(LDFLAGS) -L$(BUILD_DIR) -lross
