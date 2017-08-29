include ../../src/config.mk

all: traffic
clean:
	rm -f traffic charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv


traffic: traffic.C traffic.h intersection.C intersection.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $(OPTIONS) $(OPTS) -module CommonLBs traffic.C intersection.C -o traffic $(LDFLAGS) -L$(BUILD_DIR) -lcharades
