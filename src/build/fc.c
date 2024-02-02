
#include "../all.h"

Fc* fc_make(Nsc* nsc, char* path) {
    Pkc* pkc = nsc->pkc;
    Build* b = pkc->b;
    Allocator* alc = b->alc;

    bool is_header = ends_with(path, ".vh");

    Fc* fc = al(alc, sizeof(Fc));
    fc->b = b;
    fc->path = path;
    fc->alc = alc;
    fc->nsc = nsc;
    fc->scope = scope_make(alc, nsc->scope);
    fc->is_header = is_header;

    fc->funcs = array_make(alc, 8);
    fc->classes = array_make(alc, 4);
    fc->aliasses = array_make(alc, 8);

    // Load content
    if(!file_exists(path)) {
        sprintf(b->char_buf, "File not found: '%s'", path);
        build_err(b, b->char_buf);
    }

    usize start = microtime();
    Str* content_str = str_make(alc, 512);
    file_get_contents(content_str, path);
    b->time_io += microtime() - start;

    Chunk* content = chunk_make(alc, b, fc);
    chunk_set_content(content, str_to_chars(alc, content_str), content_str->length);
    fc->content = content;
    fc->chunk_parse = content;
    fc->chunk_parse_prev = chunk_clone(alc, content);

    stage_add_item(b->stage_1_parse, fc);

    return fc;
}
