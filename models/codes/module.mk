include ../config.mk

#C_FILES=$(wildcard *.C)
CPP_FILES=$(wildcard *.cpp)
#CI_FILES=$(wildcard *.ci)
#MODULES=$(CI_FILES:.ci=.decl.h)
#OBJS=$(addprefix $(CODES_BUILD)/, $(C_FILES:.C=.o) $(CPP_FILES:.cpp=.o))
OBJS=$(addprefix $(CODES_BUILD)/, $(CPP_FILES:.cpp=.o))

default: all
all: $(CODES_BUILD) $(OBJS)

$(CODES_BUILD):
	mkdir $(CODES_BUILD)

$(CODES_BUILD)/%.o: %.cpp %.h $(MODULES)
	$(CHARMC) $(CFLAGS) -c $< -o $@

$(CODES_BUILD)/%.o: %.cpp $(MODULES)
	$(CHARMC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)
