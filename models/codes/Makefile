include ../../src/config.mk

OBJS = $(patsubst %.cpp, %.o, $(shell find . -name "*.cpp"))

CFLAGS += -I.

all: codes_exe

codes_exe: $(OBJS) $(BUILD_DIR)/libcharades.a
	$(CHARMC) $(LFLAGS) -o $@ $^ -L$(BUILD_DIR) -lcharades

%.o: %.cpp
	$(CHARMC) $(CFLAGS) $^ -o $@

clean:
	rm -f codes_exe charmrun
	rm -f $(OBJS)
