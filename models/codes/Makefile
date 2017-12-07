include ../../src/config.mk

OBJS = $(patsubst %.cpp, %.o, $(shell find . -name "*.cpp"))

CFLAGS += -I.

all: codes_exe
clean:
	rm -f codes_exe charmrun
	rm -f $(OBJS)


codes_exe: $(OBJS)
	$(CHARMC) $(LFLAGS) -o $@ $^ -L$(BUILD_DIR) -lcharades

%.o: %.cpp
	$(CHARMC) $(CFLAGS) $^ -o $@
