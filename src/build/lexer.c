
#include "../all.h"

void chunk_lex(Build* b, Chunk *chunk, ChunkPos* err_pos) {
    //
    char *content = chunk->content;
    int length = chunk->length;
    Fc *fc = chunk->fc;

    if (b->verbose > 2 && fc && !err_pos) {
        printf("Lex: %s\n", fc->path);
    }

    int i = 0;
    int o = 0;
    int depth = 0;
    char closer_chars[256];
    int closer_indexes[256];
    char bracket_table[128];
    bracket_table['('] = ')';
    bracket_table['['] = ']';
    bracket_table['{'] = '}';
    bracket_table['<'] = '}';

    int cc_depth = 0;

    char token[512];
    int token_i = 0;

    int line = 1;
    int col = 0;
    int i_last = 0;

    Str* str_buf = b->str_buf;
    str_preserve(str_buf, length * 2 + 1024);
    char *tokens = str_buf->data;

    while (true) {
        const char ch = content[i];
        if (err_pos && o >= err_pos->i) {
            err_pos->content_i = i;
            err_pos->line = line;
            err_pos->col = col;
            return;
        }
        if (ch == 0)
            break;
        i++;
        col += i - i_last;
        i_last = i;
        // Make sure we have enough memory
        if (str_buf->mem_size - o < 512) {
            str_buf->length = o;
            str_increase_memsize(str_buf, str_buf->mem_size * 2);
            tokens = str_buf->data;
        }
        // Spaces, newlines & comments
        if (ch <= 32 || (ch == '/' && content[i] == '/')) {
            bool has_newline = false;
            i--;
            while (true) {
                char ch = content[i++];
                if (ch == ' ' || ch == '\t') {
                    continue;
                }
                if (ch == '\n') {
                    has_newline = true;
                    i_last = i;
                    col = 0;
                    line++;
                    continue;
                }
                if (ch == '/' && content[i] == '/') {
                    i++;
                    ch = content[i++];
                    while (ch != '\n' && ch != 0) {
                        ch = content[i++];
                    }
                    if(ch == '\n')
                        i--;
                }
                if (ch == 0)
                    break;
                if (ch <= 32)
                    continue;
                break;
            }
            i--;
            tokens[o++] = has_newline ? tok_newline : tok_space;
            continue;
        }

        // // Compile conditions
        // if (ch == '#' && (o < 2 || tokens[o - 2] == tok_newline)) {
        //     int x = i;
        //     char ch = content[x];
        //     while (ch >= 97 && ch <= 122) {
        //         token[token_i++] = ch;
        //         ch = content[++x];
        //     }
        //     token[token_i] = '\0';
        //     token_i = 0;
        //     if (str_is(token, "if")) {
        //         tokens[o++] = tok_cc;
        //         strcpy((char *)((intptr_t)tokens + o), token);
        //         o += 3;
        //         cc_depth++;
        //         i = x;
        //         continue;
        //     }
        //     if (str_is(token, "elif") || str_is(token, "else") || str_is(token, "end")) {
        //         tokens[o++] = tok_cc;
        //         strcpy((char *)((intptr_t)tokens + o), token);
        //         o += (str_is(token, "end")) ? 4 : 5;
        //         if (str_is(token, "end")) {
        //             cc_depth--;
        //             if (cc_depth < 0) {
        //                 chunk->i = i;
        //                 sprintf(b->char_buf, "Using #%s without an #if before it", token);
        //                 parse_err(chunk, b->char_buf);
        //             }
        //         }
        //         i = x;
        //         continue;
        //     }
        // }
        // ID: a-zA-Z_
        if ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || ch == 95) {

            tokens[o++] = tok_id;
            tokens[o++] = tok_data_with_pos;
            *(void **)((intptr_t)tokens + o) = content + i - 1;
            o += sizeof(void*);
            *(int *)((intptr_t)tokens + o) = line;
            o += sizeof(int);
            *(int *)((intptr_t)tokens + o) = col;
            o += sizeof(int);

            char ch = content[i];
            while ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || (ch >= 48 && ch <= 57) || ch == 95) {
                ch = content[++i];
            }
            continue;
        }
        // Number
        if (ch >= 48 && ch <= 57) {
            tokens[o++] = tok_number;
            tokens[o++] = tok_data;
            *(void **)((intptr_t)tokens + o) = content + i - 1;
            o += sizeof(void*);

            // 0-9
            char ch = content[i];
            while (ch >= 48 && ch <= 57) {
                ch = content[++i];
            }
            continue;
        }
        // Strings
        if (ch == '"') {
            int start = i;
            tokens[o++] = tok_string;
            tokens[o++] = tok_data;
            *(void **)((intptr_t)tokens + o) = content + i;
            o += sizeof(void*);

            char ch = content[i];
            while (ch != '"') {
                if (ch == '\\') {
                    ch = content[++i];
                }
                if (ch == 0) {
                    lex_err(chunk, start, "Missing string closing tag '\"', compiler reached end of file");
                }
                ch = content[++i];
                // Extend memory if needed
                if ((o % 200 == 0) && str_buf->mem_size - o < 512) {
                    str_buf->length = o;
                    str_increase_memsize(str_buf, str_buf->mem_size * 2);
                    tokens = str_buf->data;
                }
            }
            i++;
            continue;
        }
        if (ch == '\'') {
            char ch = content[i++];
            if (ch == '\\') {
                ch = content[i++];
                ch = backslash_char(ch);
            }
            if (content[i++] != '\'') {
                lex_err(chunk, i, "Missing character closing tag ('), found '%c'", content[i - 1]);
            }
            tokens[o++] = tok_char;
            tokens[o++] = tok_data_i8;
            tokens[o++] = ch;
            continue;
        }
        // Scopes
        if (ch == '(' || ch == '[' || ch == '{' || (ch == '<' && content[i] == '{')) {
            tokens[o++] = tok_scope_open;
            tokens[o++] = tok_data_with_i32;

            *(void **)((intptr_t)tokens + o) = content + i - 1;
            o += sizeof(void*);
            int index = o;
            o += sizeof(int);

            if (ch == '<') {
                i++;
            }
            closer_chars[depth] = bracket_table[ch];
            closer_indexes[depth] = index;
            depth++;
            continue;
        }
        if (ch == ')' || ch == ']' || ch == '}') {
            depth--;
            if (depth < 0) {
                lex_err(chunk, "Unexpected closing tag '%c'", ch);
            }
            if (closer_chars[depth] != ch) {
                lex_err(chunk, "Unexpected closing tag '%c', expected '%c'", ch, closer_chars[depth]);
            }
            tokens[o++] = tok_scope_close;
            tokens[o++] = tok_data_i8;
            tokens[o++] = ch;
            int offset = closer_indexes[depth];
            *(int *)(&tokens[offset]) = o;
            continue;
        }
        // =
        char op = -1;
        char next = content[i];
        if (ch == '=') {
            op = tok_eq;
            if(next == '=') {
                op = tok_eqeq;
                i++;
            }
        } else if (ch == '+') {
            op = tok_plus;
            if(next == '+') {
                op = tok_plusplus;
                i++;
            } else if(next == '=') {
                op = tok_plus_eq;
                i++;
            }
        } else if (ch == '-') {
            op = tok_sub;
            if(next == '-') {
                op = tok_subsub;
                i++;
                if(content[i] == '-') {
                    op = tok_triple_sub;
                    i++;
                }
            } else if(next == '=') {
                op = tok_sub_eq;
                i++;
            }
        } else if (ch == '*') {
            op = tok_mul;
            if(next == '=') {
                op = tok_mul_eq;
                i++;
            }
        } else if (ch == '/') {
            op = tok_div;
            if(next == '=') {
                op = tok_div_eq;
                i++;
            }
        } else if (ch == '!') {
            op = tok_not;
            if(next == '=') {
                op = tok_not_eq;
                i++;
            }
        } else if (ch == '>') {
            op = tok_gt;
            if(next == '=') {
                op = tok_gte;
                i++;
            } else if(next == '>') {
                op = tok_shr;
                i++;
            }
        } else if (ch == '<') {
            op = tok_lt;
            if(next == '=') {
                op = tok_lte;
                i++;
            } else if(next == '<') {
                op = tok_shl;
                i++;
            }
        } else if (ch == '&') {
            op = tok_bit_and;
            if(next == '&') {
                op = tok_and;
                i++;
            }
        } else if (ch == '|') {
            op = tok_bit_or;
            if(next == '|') {
                op = tok_or;
                i++;
            }
        } else if (ch == '?') {
            op = tok_qmark;
        } else if (ch == '^') {
            op = tok_bit_xor;
        } else if (ch == ':') {
            op = tok_colon;
        } else if (ch == '.') {
            op = tok_dot;
        } else if (ch == '~') {
            op = tok_tilde;
        } else if (ch == '#') {
            op = tok_hashtag;
        } else if (ch == ';') {
            op = tok_semi;
        } else if (ch == ',') {
            op = tok_comma;
        } else if (ch == '@') {
            op = tok_at;
        }

        if(op > -1) {
            tokens[o++] = op;
            continue;
        }

        lex_err(chunk, i, "Unexpected character '%c' (byte:%d, pos:%d)", ch, ch, i);
    }

    if (depth > 0) {
        lex_err(chunk, i, "Missing closing tag '%c'", closer_chars[depth - 1]);
    }

    tokens[o++] = tok_eof;
    tokens[o++] = '\0';

    str_buf->length = o;

    chunk->tokens = str_to_chars(b->alc, str_buf);
    b->LOC += line;

    // Probably will never happen
    // if (err_token_i > -1) {
    //     *err_content_i = i;
    //     *err_line = line;
    //     *err_col = col;
    //     return;
    // }
}
