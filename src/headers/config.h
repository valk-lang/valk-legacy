
#ifndef _H_CONFIG
#define _H_CONFIG

#include "typedefs.h"
#include "cJSON.h"

PkgConfig* load_config(Allocator* alc, char* dir, Str* buf, bool must_exist);
bool cfg_has_package(PkgConfig *cfg, char *name);
char* cfg_get_pkg_dir(PkgConfig *cfg, char *name, Allocator* alc);
char* cfg_get_nsc_dir(PkgConfig *cfg, char *name, Allocator* alc);
Array* cfg_get_header_dirs(PkgConfig *cfg, Allocator* alc, char* pkg_dir);

struct PkgConfig {
    char* path;
    char* dir;
    cJSON* json;
};

#endif
