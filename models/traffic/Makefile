include ../../src/config.mk

all: traffic
clean:
	rm -f traffic charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv


traffic: traffic.C traffic.h $(BUILD_DIR)/libross.a
	$(CHARMC) $(CFLAGS) traffic.C -o traffic $(LFLAGS) -L$(BUILD_DIR) -lross
