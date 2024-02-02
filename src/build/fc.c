
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
    fc->is_header = is_header;

    // Load content
    if(!file_exists(path)) {
        sprintf(b->char_buf, "File not found: '%s'", path);
        build_err(b, b->char_buf);
    }

    Str* content_str = str_make(alc, 512);
    file_get_contents(content_str, path);

    Chunk* content = chunk_make(alc, b, fc);
    chunk_set_content(content, str_to_chars(alc, content_str), content_str->length);
    fc->content = content;
    fc->chunk_parse = content;

    return fc;
}
