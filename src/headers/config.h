
#ifndef _H_CONFIG
#define _H_CONFIG

#include "typedefs.h"
#include "cJSON.h"

PkgConfig* load_config(Allocator* alc, char* path, Str* buf, bool must_exist);
bool cfg_has_package(PkgConfig *cfg, char *name);
char* cfg_get_pkg_dir(PkgConfig *cfg, char *name, Allocator* alc);
void cfg_get_header_dirs(PkgConfig *cfg, Allocator* alc, char* pkg_dir, Array* result);

struct PkgConfig {
    char* path;
    cJSON* json;
};

#endif
