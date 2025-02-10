
#include "../all.h"

void die(char *msg) {
    printf("%s\n", msg);
    exit(1);
}

void parse_argv(char **argv, int argc, Allocator* alc, Array *has_value, Array *args, Map *options) {
    //
    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];
        int index = array_find(has_value, arg, arr_find_str);
        if (index == -1) {
            array_push(args, arg);
            continue;
        }
        i++;
        if (i == argc) {
            break;
        }
        char *value = argv[i];

        if(str_is(arg, "-L")) {
            Array* arr = map_get(options, arg);
            if (arr == NULL) {
                arr = array_make(alc, 10);
                map_set(options, arg, arr);
            }
            array_push(arr, value);
        } else {
            map_set(options, arg, value);
        }
    }
}

unsigned long microtime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    return time_in_micros;
}

#ifdef WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
int gettimeofday(struct timeval *tv, void *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  /*convert into microseconds*/
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  return 0;
}
#endif

void sleep_ms(unsigned int ms) {
#ifdef WIN32
    Sleep(ms);
#else
    //
    struct timespec ts;
    int res;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
#endif
}

unsigned int ctxhash_u32(char *content) {
    int len = 32;
    char buf[33];
    ctxhash(content, buf);
    unsigned int result = 0;
    unsigned char* ref = (unsigned char*)&result;
    while(len-- > 0) {
        ref[len % 4] += buf[len];
    }
    return result;
}

// simple hash has similar speed to crc32 but returns a string instead of a number
// by: github.com/ctxcode
void ctxhash(char *content_, char *buf_) {

    unsigned char *content = (unsigned char *)content_;
    unsigned char *buf = (unsigned char *)buf_;

    const int hash_len = 32;

    memset(buf, '\0', hash_len);

    int res_pos = 0;
    int str_pos = 0;
    unsigned char diff = 0;

    bool end = false;

    while (true) {
        unsigned char str_ch = content[str_pos++];
        if (str_ch == '\0') {
            end = true;
            str_pos = 0;
            continue;
        }

        diff += (str_ch + str_pos) * 0b00010101 + res_pos;
        buf[res_pos++] += str_ch + diff;

        if (res_pos == hash_len) {
            if (end)
                break;
            res_pos = 0;
        }
    }

    const char *chars = "TMpUivZnQsHw1klS3Ah5d6qr7tjKxJOIEmYP8VgGzcDR0f2uBe4aobWLNCFy9X";

    int i = hash_len;
    while (i-- > 0) {
        const unsigned char str_ch = buf[i];
        diff += (str_ch + i) * 0b0001011 + i;
        buf[i] = chars[(str_ch + diff) % 62];
    }
    buf[hash_len] = '\0';
}

size_t get_mem_usage() {
    #ifdef WIN32
    return 0;
    #elif __APPLE__
    return 0;
    #else
    struct mallinfo2 info = mallinfo2();
    return info.arena + info.hblkhd;
    #endif
}

char* arch_str(int arch) {
    if(arch == arch_x64)
        return "x64";
    else if(arch == arch_arm64)
        return "arm64";
    return "unknown";
}

char* os_str(int os) {
    if(os == os_win)
        return "win";
    else if(os == os_linux)
        return "linux";
    else if(os == os_macos)
        return "macos";
    return "unknown";
}

