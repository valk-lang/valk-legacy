
#include "../all.h"

Fc *fc_make(Nsc *nsc, char *path) {
    Pkc *pkc = nsc->pkc;
    Build *b = pkc->b;
    Allocator *alc = b->alc;

    bool is_header = ends_with(path, ".vh");

    Fc *fc = al(alc, sizeof(Fc));
    fc->b = b;
    fc->path = path;
    fc->alc = alc;
    fc->nsc = nsc;
    fc->scope = scope_make(alc, sc_default, nsc->scope);
    fc->is_header = is_header;

    fc->funcs = array_make(alc, 8);
    fc->classes = array_make(alc, 4);
    fc->aliasses = array_make(alc, 8);
    fc->globals = array_make(alc, 8);

    fc->hash = NULL;

    fc->ir_changed = false;

    // Paths
    char *path_ir = al(alc, VOLT_PATH_MAX);
    char *path_cache = al(alc, VOLT_PATH_MAX);
    char *fn = get_path_basename(alc, path);
    fn = strip_ext(alc, fn);
    sprintf(path_ir, "%s%s_%s_%s.ir", b->cache_dir, nsc->name, fn, nsc->pkc->name);
    sprintf(path_cache, "%s%s_%s_%s.json", b->cache_dir, nsc->name, fn, nsc->pkc->name);
    fc->path_ir = path_ir;
    fc->path_cache = path_cache;

    // Load content
    if (!file_exists(path)) {
        sprintf(b->char_buf, "File not found: '%s'", path);
        build_err(b, b->char_buf);
    }

    usize start = microtime();
    Str *content_str = str_make(alc, 512);
    file_get_contents(content_str, path);
    usize time = microtime() - start;
    b->time_io += time;
    if(b->parser_started) b->time_parse -= time;

    Chunk *content = chunk_make(alc, b, fc);
    chunk_set_content(content, str_to_chars(alc, content_str), content_str->length);
    fc->content = content;
    fc->chunk_parse = chunk_clone(alc, content);
    fc->chunk_parse_prev = chunk_clone(alc, content);

    array_push(nsc->fcs, fc);

    stage_add_item(b->stage_1_parse, fc);

    return fc;
}
