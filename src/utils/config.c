
#include "../all.h"

PkgConfig* load_config(Allocator* alc, char* path, Str* buf, bool must_exist) {
    PkgConfig* cfg = al(alc, sizeof(PkgConfig));
    cfg->path = path;

    char msg[VOLT_PATH_MAX + 100];

    if(!file_exists(path)) {
        if(!must_exist)
            return NULL;
        sprintf(msg, "Package config not found: '%s'", path);
        die(msg);
    }

    file_get_contents(buf, path);
    char *content = str_to_chars(alc, buf);

    cJSON *json = cJSON_ParseWithLength(content, buf->length);
    if (!json) {
        sprintf(msg, "Package config contains invalid json syntax: '%s'", path);
        die(msg);
    }

    return cfg;
}

bool cfg_has_package(PkgConfig *cfg, char *name) {
    const cJSON *pkgs = cJSON_GetObjectItemCaseSensitive(cfg->json, "packages");
    if (!pkgs)
        return false;
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(pkgs, name);
    if (!item)
        return false;
    return true;
}
char* cfg_get_pkg_dir(PkgConfig *cfg, char *name, Allocator* alc) {
    const cJSON *pkgs = cJSON_GetObjectItemCaseSensitive(cfg->json, "packages");
    if (!pkgs)
        return NULL;
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(pkgs, name);
    if (!item)
        return NULL;
    // TODO
    return NULL;
}

void cfg_get_header_dirs(PkgConfig *cfg, Allocator* alc, char* pkg_dir, Array* result) {
    const cJSON *headers = cJSON_GetObjectItemCaseSensitive(cfg->json, "headers");
    if (!headers)
        return;
    cJSON *dirs = cJSON_GetObjectItemCaseSensitive(headers, "dirs");
    if (!dirs)
        return;
    char fullpath[VOLT_PATH_MAX];
    cJSON *cdir = dirs->child;
    while (cdir) {
        strcpy(fullpath, pkg_dir);
        strcat(fullpath, cdir->valuestring);
        fix_slashes(fullpath, true);

        if (!file_exists(fullpath)) {
            printf("Header directory not found: %s => (%s)\n", cdir->valuestring, fullpath);
            exit(1);
        }

        array_push(result, dups(alc, fullpath));
        cdir = cdir->next;
    }
}

void cfg_save(PkgConfig *cfg) {
    char *content = cJSON_Print(cfg->json);
    write_file(cfg->path, content, false);
    free(content);
}
