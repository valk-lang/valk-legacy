
#include "../all.h"

char tok(Parser* p, bool allow_space, bool allow_newline, bool update) {
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
    }
    char td = tokens[i];
    if(td > 240) {
        i++;
        // Data token
        if(td == tok_data) {
            p->data = *(void **)(tokens + i);
            i += sizeof(void *);
        } else if(td == tok_data_i32) {
            p->data_i32 = *(int *)(tokens + i);
            i += 4;
        } else if(td == tok_data_i8) {
            p->data_i8 = *(char *)(tokens + i);
            i++;
        } else if(td == tok_data_pos_with_chars) {
            p->line = *(int*)(tokens + i);
            i += sizeof(int);
            p->col = *(int*)(tokens + i);
            i += sizeof(int);
            p->data = (tokens + i);
            while(tokens[i++] != 0) {}
        } else if(td == tok_data_chars) {
            p->data = (tokens + i);
            while(tokens[i++] != 0) {}
        }
    }
    if(update) {
        ch->i = i;
    }

    return t;
}

void tok_expect(Parser* p, char* expect, bool allow_space, bool allow_newline) {
    int start = p->chunk->i;
    char t = tok(p, allow_space, allow_newline, true);
    char* tkn = parser_data_str(p);

    if(!str_is(tkn, expect)) {
        parse_err(p, start, "Expected '%s' here, instead of '%s'", expect, tkn);
    }
}
void tok_expect_two(Parser* p, char* expect_1, char* expect_2, bool allow_space, bool allow_newline) {
    int start = p->chunk->i;
    char t = tok(p, allow_space, allow_newline, true);
    char* tkn = parser_data_str(p);

    if(!str_is(tkn, expect_1) && !str_is(tkn, expect_2)) {
        parse_err(p, start, "Expected '%s' or '%s' here, instead of '%s'", expect_1, expect_2, tkn);
    }
}

char *tok_to_str(Allocator* alc, int t) {
}

// char tok_read_byte(Fc* fc, int offset) {
//     Chunk* ch = fc->chunk_parse;
//     return ch->tokens[ch->i + offset];
// }
// char tok_id_next(Fc* fc) {
//     Chunk* ch = fc->chunk_parse;
//     return ch->tokens[ch->i];
// }
// char tok_id_next_ignore_spacing(Fc* fc) {
//     Chunk* ch = fc->chunk_parse;
//     int t = ch->tokens[ch->i];
//     if(t == tok_space || t == tok_newline) 
//         return ch->tokens[ch->i + 2];
//     return t;
// }
// void tok_skip_whitespace(Fc* fc) {
//     Chunk* ch = fc->chunk_parse;
//     int t = ch->tokens[ch->i];
//     if(t == tok_space || t == tok_newline) {
//         ch->i += 2;
//     }
// }
// void tok_skip_space(Fc* fc) {
//     Chunk* ch = fc->chunk_parse;
//     int t = ch->tokens[ch->i];
//     if(t == tok_space) {
//         ch->i += 2;
//     }
// }
// bool tok_next_is_whitespace(Fc* fc) {
//     Chunk* ch = fc->chunk_parse;
//     int t = ch->tokens[ch->i];
//     if(t == tok_space || t == tok_newline) {
//         return true;
//     }
//     return false;
// }

// char* chunk_tok(Chunk* chunk, bool allow_space, bool allow_newline, bool update) {
//     int x = chunk->i;
//     char* res = chunk_read(chunk, &x);
//     char t = chunk->token;
//     if(t == tok_space) {
//         if(!allow_space) {
//             chunk->token = tok_none;
//             return "";
//         }
//         res = chunk_read(chunk, &x);
//         t = chunk->token;
//     }
//     if(t == tok_newline) {
//         if(!allow_space || !allow_newline){
//             chunk->token = tok_none;
//             return "";
//         }
//         res = chunk_read(chunk, &x);
//     }
//     if(update) {
//         chunk->i = x;
//     }
//     return res;
// }

// char* chunk_read(Chunk* chunk, int *i_ref) {
//     //
//     int i = i_ref ? *i_ref : chunk->i;
//     char* tokens = chunk->tokens;
//     char t = tokens[i++];
//     if(t == tok_pos) {
//         int line = *(int*)(&tokens[i]);
//         i += sizeof(int);
//         int col = *(int*)(&tokens[i]);
//         i += sizeof(int);
//         chunk->line = line;
//         chunk->col = col;
//         t = tokens[i++];
//     }
//     if(t == tok_scope_open) {
//         chunk->scope_end_i = *(int*)(&tokens[i]);
//         i += sizeof(int);
//     }
//     char* tchars = (char*)(&tokens[i]);
//     if(t != tok_eof && i_ref) {
//         while(tokens[i++] != 0) {
//         }
//         *i_ref = i;
//     }
//     chunk->token = t;
//     return tchars;
// }
