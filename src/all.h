
#ifndef _H_ALL
#define _H_ALL

#ifdef _WIN32
#pragma clang diagnostic ignored "-Wpragma-pack"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wignored-pragma-intrinsic"
#pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
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

#if __APPLE__
#else
#include <malloc.h>
#endif

////////////////////////////

#define VALK_PATH_MAX 4096
#define VALK_TOKEN_MAX 256
#define PATH_SLASH_CHAR '/'
#define PATH_SLASH "/"

#ifndef VALK_VERSION
#define VALK_VERSION "no_version"
#endif

#define max_num(x, y) (((x) >= (y)) ? (x) : (y))
#define min_num(x, y) (((x) <= (y)) ? (x) : (y))
#define str_flat(str, chars) str_append_chars(str, chars)

#define v_i64 long long int
#define v_u64 unsigned long long int

// Base
#include "headers/enums.h"
#include "headers/typedefs.h"
#include "headers/structs.h"
#include "headers/functions.h"

// Globals
extern Array* watch_dirs;
extern bool is_watching;

// Utils
#include "headers/cJSON.h"
#include "headers/alloc.h"
#include "headers/array.h"
#include "headers/map.h"
#include "headers/files.h"
#include "headers/syntax.h"
#include "headers/str.h"
#include "headers/config.h"
// Commands
#include "headers/build.h"
// Build
#include "headers/parse.h"
#include "headers/token.h"
#include "headers/value.h"
#include "headers/id.h"
#include "headers/type.h"
#include "headers/skip.h"
#include "headers/func.h"
#include "headers/class.h"
#include "headers/ir.h"
#include "headers/snippet.h"
#include "headers/chunk.h"
#include "headers/parser.h"
#include "headers/unit.h"
#include "headers/error.h"
#include "headers/scope.h"
#include "headers/pool.h"
#include "headers/compile-cond.h"
#include "headers/test.h"
#include "headers/thread.h"
#include "headers/macro.h"
#include "headers/coro.h"

#endif
