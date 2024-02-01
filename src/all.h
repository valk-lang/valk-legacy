
#ifndef _H_ALL
#define _H_ALL

#ifdef _WIN32
#include <windows.h>
#else
// #include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
// #include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

////////////////////////////

#define VOLT_PATH_MAX 4096
#define VOLT_TOKEN_MAX 256

#define max_num(x, y) (((x) >= (y)) ? (x) : (y))
#define min_num(x, y) (((x) <= (y)) ? (x) : (y))

#include "headers/typedefs.h"
#include "headers/structs.h"
#include "headers/functions.h"

#include "headers/alloc.h"
#include "headers/array.h"
#include "headers/map.h"

#include "headers/build.h"

#endif
