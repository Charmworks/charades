// TODO: Rather than static globals we should move this to a group chare
// holding all of the globals/configuration variables.
static const tw_optdef* opt_groups[10];
static unsigned int opt_index = 0;

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
  opt_groups[opt_index++] = options;
  opt_groups[opt_index] = NULL;
}

void tw_opt_print() {}
