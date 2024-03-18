
#include "../all.h"

char tok(Parser* p, bool allow_space, bool allow_newline, bool update) {
    p->tkn = "";
    Chunk* ch = p->chunk;
    char *tokens = ch->tokens;
    int i = ch->i;
    char t = tokens[i++];
    if(t == tok_space) {
        if (!allow_space)
            return tok_none;
        t = tokens[i++];
    }
    if(t == tok_newline) {
        if (!allow_newline || !allow_space)
            return tok_none;
        t = tokens[i++];
        p->on_newline = true;
    } else {
        p->on_newline = false;
    }
    char td = tokens[i];
    if(td == 2 || td == 3) {
        i++;
        // Data token
        if(td == tok_data_pos) {
            p->line = *(int*)(tokens + i);
            i += sizeof(int);
            p->col = *(int*)(tokens + i);
            i += sizeof(int);
        } else if(td == tok_data_scope_end) {
            p->scope_end_i = *(int*)(tokens + i);
            i += sizeof(int);
        }
    }
    // Token chars
    p->tkn = (tokens + i);

    //
    if(update) {
        // Skip tkn bytes
        while(tokens[i++] != 0) {}
        //
        ch->i = i;
    }

    return t;
}

void tok_expect(Parser* p, char* expect, bool allow_space, bool allow_newline) {
    int start = p->chunk->i;
    char t = tok(p, allow_space, allow_newline, true);
    char* tkn = p->tkn;
    if(!str_is(tkn, expect)) {
        parse_err(p, start, "Expected '%s' here, instead of '%s'", expect, tkn);
    }
}
char tok_expect_two(Parser* p, char* expect_1, char* expect_2, bool allow_space, bool allow_newline) {
    int start = p->chunk->i;
    char t = tok(p, allow_space, allow_newline, true);
    char* tkn = p->tkn;
    if(!str_is(tkn, expect_1) && !str_is(tkn, expect_2)) {
        parse_err(p, start, "Expected '%s' or '%s' here, instead of '%s'", expect_1, expect_2, tkn);
    }
    return t;
}

void tok_expect_newline(Parser* p) {
    char* tokens = p->chunk->tokens;
    int i = p->chunk->i;
    char t = tokens[i];
    if(t == tok_space) {
        i++;
    }
    t = tokens[i];
    if(t != tok_newline) {
        parse_err(p, -1, "Expected a new line here");
    }
    p->chunk->i = i + 1;
}

bool tok_next_is_whitespace(Parser* p) {
    int t = p->chunk->tokens[p->chunk->i];
    return t == tok_space || t == tok_newline;
}
