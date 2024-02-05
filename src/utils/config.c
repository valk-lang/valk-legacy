
#include "../all.h"

PkgConfig* load_config(Allocator* alc, char* dir, Str* str_buf, bool must_exist) {

    char buf[VOLT_PATH_MAX + 100];
    strcpy(buf, dir);
    strcat(buf, "volt.json");
    char* path = dups(alc, buf);

    PkgConfig* cfg = al(alc, sizeof(PkgConfig));
    cfg->dir = dir;
    cfg->path = path;

    if(!file_exists(path)) {
        if(!must_exist)
            return NULL;
        sprintf(buf, "Package config not found: '%s'", path);
        die(buf);
    }

    file_get_contents(str_buf, path);
    char *content = str_to_chars(alc, str_buf);

    cJSON *json = cJSON_ParseWithLength(content, str_buf->length);
    if (!json) {
        sprintf(buf, "Package config contains invalid json syntax: '%s'", path);
        die(buf);
    }

    cfg->json = json;
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
char* cfg_get_nsc_dir(PkgConfig *cfg, char *name, Allocator* alc) {
    const cJSON *spaces = cJSON_GetObjectItemCaseSensitive(cfg->json, "namespaces");
    if (!spaces)
        return NULL;
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(spaces, name);
    if (!item || !cJSON_IsString(item))
        return NULL;
    char* sub = item->valuestring;
    char buf[VOLT_PATH_MAX];
    strcpy(buf, cfg->dir);
    strcat(buf, sub);
    fix_slashes(buf, true);
    return dups(alc, buf);
}

Array* cfg_get_header_dirs(PkgConfig *cfg, Allocator* alc, char* pkg_dir) {

    Array* result = array_make(alc, 10);

    const cJSON *headers = cJSON_GetObjectItemCaseSensitive(cfg->json, "headers");
    if (!headers)
        return result;
    cJSON *dirs = cJSON_GetObjectItemCaseSensitive(headers, "dirs");
    if (!dirs)
        return result;

    char fullpath[VOLT_PATH_MAX];
    cJSON *cdir = dirs->child;
    while (cdir) {
        strcpy(fullpath, pkg_dir);
        strcat(fullpath, cdir->valuestring);
        fix_slashes(fullpath, true);

        if (!file_exists(fullpath)) {
            printf("Warn: Header directory not found: %s => (%s)\n", cdir->valuestring, fullpath);
            continue;
        }

        array_push(result, dups(alc, fullpath));
        cdir = cdir->next;
    }
    return result;
}

void cfg_save(PkgConfig *cfg) {
    char *content = cJSON_Print(cfg->json);
    write_file(cfg->path, content, false);
    free(content);
}
