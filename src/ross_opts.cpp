#include "typedefs.h"

#define SET_PE_VALUE(x,y) true

static int is_empty(const tw_optdef* def) {
  for (; def->type; def++) {
    if (def->type == TWOPTTYPE_GROUP)
      continue;
    return 0;
  }
  return 1;
}

void tw_opt_parse(int* argc, char*** argv) {}

void tw_opt_add(const tw_optdef* options) {
  if (!options || !options->type || is_empty(options))
    return;
  SET_PE_VALUE(opt_groups[opt_index++], options);
  SET_PE_VALUE(opt_groups[opt_index], NULL);
}

void tw_opt_print() {}
