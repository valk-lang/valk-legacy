
#include "../all.h"

char* tok(Fc* fc, bool allow_space, bool allow_newline, bool update) {
    Chunk* ch = fc->chunk_parse;
    Chunk* prev = fc->chunk_parse_prev;
    if(update)
        *prev = *ch;
    return chunk_tok(ch, allow_space, allow_newline, update);
}
void tok_back(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    Chunk* prev = fc->chunk_parse_prev;
    *ch = *prev;
}
void tok_expect(Fc* fc, char* expect, bool allow_space, bool allow_newline) {
    char* tkn = tok(fc, allow_space, allow_newline, true);
    if(!str_is(tkn, expect)) {
        sprintf(fc->b->char_buf, "Expected '%s' here, instead of '%s'", expect, tkn);
        parse_err(fc->chunk_parse, fc->b->char_buf);
    }
}
char* tok_expect_two(Fc* fc, char* expect_1, char* expect_2, bool allow_space, bool allow_newline) {
    char* tkn = tok(fc, allow_space, allow_newline, true);
    if(!str_is(tkn, expect_1) && !str_is(tkn, expect_2)) {
        sprintf(fc->b->char_buf, "Expected '%s' or '%s' here, instead of '%s'", expect_1, expect_2, tkn);
        parse_err(fc->chunk_parse, fc->b->char_buf);
    }
    return tkn;
}
char tok_read_byte(Fc* fc, int offset) {
    Chunk* ch = fc->chunk_parse;
    return ch->tokens[ch->i + offset];
}
char tok_id_next(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    return ch->tokens[ch->i];
}
char tok_id_next_ignore_spacing(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    int t = ch->tokens[ch->i];
    if(t == tok_space || t == tok_newline) 
        return ch->tokens[ch->i + 2];
    return t;
}
void tok_skip_whitespace(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    int t = ch->tokens[ch->i];
    if(t == tok_space || t == tok_newline) {
        ch->i += 2;
    }
}
void tok_skip_space(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    int t = ch->tokens[ch->i];
    if(t == tok_space) {
        ch->i += 2;
    }
}
bool tok_next_is_whitespace(Fc* fc) {
    Chunk* ch = fc->chunk_parse;
    int t = ch->tokens[ch->i];
    if(t == tok_space || t == tok_newline) {
        return true;
    }
    return false;
}

char* chunk_tok(Chunk* chunk, bool allow_space, bool allow_newline, bool update) {
    int x = chunk->i;
    char* res = chunk_read(chunk, &x);
    char t = chunk->token;
    if(t == tok_space) {
        if(!allow_space) {
            chunk->token = tok_none;
            return "";
        }
        res = chunk_read(chunk, &x);
        t = chunk->token;
    }
    if(t == tok_newline) {
        if(!allow_space || !allow_newline){
            chunk->token = tok_none;
            return "";
        }
        res = chunk_read(chunk, &x);
    }
    if(update) {
        chunk->i = x;
    }
    return res;
}

char* chunk_read(Chunk* chunk, int *i_ref) {
    //
    int i = i_ref ? *i_ref : chunk->i;
    char* tokens = chunk->tokens;
    char t = tokens[i++];
    if(t == tok_pos) {
        int line = *(int*)(&tokens[i]);
        i += sizeof(int);
        int col = *(int*)(&tokens[i]);
        i += sizeof(int);
        chunk->line = line;
        chunk->col = col;
        t = tokens[i++];
    }
    if(t == tok_scope_open) {
        chunk->scope_end_i = *(int*)(&tokens[i]);
        i += sizeof(int);
    }
    char* tchars = (char*)(&tokens[i]);
    if(t != tok_eof && i_ref) {
        while(tokens[i++] != 0) {
        }
        *i_ref = i;
    }
    chunk->token = t;
    return tchars;
}
