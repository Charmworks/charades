include ../../src/config.mk

all: traffic

traffic: traffic.C traffic.h $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) traffic.C -o traffic $(LFLAGS) -L$(BUILD_DIR) -lcharades

clean:
	rm -f traffic charmrun
	rm -f *.o *~ core*
	rm -f output ross.csv
