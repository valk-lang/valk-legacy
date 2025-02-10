
// Functions
void die(char *msg);
void parse_argv(char **argv, int argc, Allocator* alc, Array *has_value, Array *args, Map *options);
unsigned long microtime();
void sleep_ms(unsigned int ms);
unsigned int ctxhash_u32(char *content);
void ctxhash(char *content_, char *buf_);
size_t get_mem_usage();
char* arch_str(int arch);
char* os_str(int os);

#ifdef WIN32
int gettimeofday(struct timeval *tv, void *tz);
#endif
