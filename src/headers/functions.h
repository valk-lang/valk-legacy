
// Functions
void die(char *msg);
void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options);
unsigned long microtime();
void ctxhash(char *content_, char *buf_);
int ipow(int base, int exp);
size_t get_mem_usage();

