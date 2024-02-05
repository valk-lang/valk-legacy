
#include "../all.h"

void die(char *msg) {
    printf("%s\n", msg);
    exit(1);
}

void parse_argv(char **argv, int argc, Array *has_value, Array *args, Map *options) {
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
        map_set(options, arg, value);
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

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;

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

  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }

  return 0;
}
#endif

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
            if (end) {
                break;
            }
            res_pos = 0;
        }
    }

    const char *chars = "TMpUivZnQsHw1klS3Ah5d6qr7tjKxJOIEmYP8VgGzcDR0f2uBe4aobWLNCFy9X";

    int i = hash_len;
    while (i > 0) {
        i--;

        const unsigned char str_ch = buf[i];
        diff += (str_ch + i) * 0b0001011 + i;
        buf[i] = chars[(str_ch + diff) % 62];
    }
    buf[hash_len] = '\0';
}
