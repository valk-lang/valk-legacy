
#include "../all.h"

Fc *fc_make(Nsc *nsc, char *path, bool is_sub_header) {
    Pkc *pkc = nsc->pkc;
    Build *b = pkc->b;
    Allocator *alc = b->alc;

    bool is_header = ends_with(path, ".vh");

    Fc *fc = al(alc, sizeof(Fc));
    fc->b = b;
    fc->path = path;
    fc->alc = alc;
    fc->nsc = nsc;
    fc->header_pkc = NULL;
    fc->scope = scope_make(alc, sc_default, nsc->scope);
    fc->is_header = is_header;
    fc->ignore_access_types = false;

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
    chunk_set_content(b, content, str_to_chars(alc, content_str), content_str->length);
    fc->content = content;

    if(!is_sub_header)
        stage_add(b, stage_prio_fc, stage_fc, fc);

    return fc;
}
