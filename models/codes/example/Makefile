include ../config.mk

CPP_FILES=$(wildcard *.cpp)
OBJS=$(addprefix $(CODES_BUILD)/, $(CPP_FILES:.cpp=.o))

default: all
all: example

example: example.cpp $(CODES_BUILD)/libcodes.a $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(CFLAGS) $(LFLAGS) example.cpp -o $@ -L$(BUILD_DIR) -L$(CODES_BUILD) -lcharades -lcodes

test: example
	./charmrun +p4 ++local ./example --codes-config=example.conf --sync=1 --end=1024 --expected-events=2032
	./charmrun +p4 ++local ./example --codes-config=example.conf --sync=2 --end=1024 --expected-events=2032
	./charmrun +p4 ++local ./example --codes-config=example.conf --sync=3 --end=1024 --expected-events=2032

clean:
	rm -rf example charmrun
