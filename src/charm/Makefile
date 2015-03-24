include ../config.mk

all: $(BUILD_DIR)/pe.o $(BUILD_DIR)/lp.o $(BUILD_DIR)/event.o $(BUILD_DIR)/ross_core.o

ci_files: ross_core.ci pe.ci lp.ci event.ci
	$(CHARMC) ross_core.ci pe.ci lp.ci event.ci
	touch ci_files

$(BUILD_DIR)/ross_core.o: ci_files ross_core.h ross_core.C
	$(CHARMC) $(OPTS) -c ross_core.C -o $@

$(BUILD_DIR)/pe.o: ci_files pe.h pe.C
	$(CHARMC) $(OPTS) -c pe.C -o $@

$(BUILD_DIR)/lp.o: ci_files lp.h lp.C
	$(CHARMC) $(OPTS) -c lp.C -o $@

$(BUILD_DIR)/event.o: ci_files event.h event.C
	$(CHARMC) $(OPTS) -c event.C -o $@

clean:
	rm -rf *.decl.h *.def.h ci_files
