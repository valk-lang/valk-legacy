
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
    char bracket_token[256];
    bracket_token['('] = tok_bracket_open;
    bracket_token['['] = tok_sq_bracket_open;
    bracket_token['{'] = tok_curly_open;
    bracket_token['<'] = tok_ltcurly_open;
    bracket_token[')'] = tok_bracket_close;
    bracket_token[']'] = tok_sq_bracket_close;
    bracket_token['}'] = tok_curly_close;

    int cc_depth = 0;

    char token[512];
    int token_i = 0;

    int line = 1;
    int col = 0;
    int i_last = 0;

    Str* str_buf = b->str_buf;
    str_clear(str_buf);
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

        // ID: a-zA-Z_
        if ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || ch == 95 || (ch == '@' && is_valid_varname_first_char(content[i]))) {

            tokens[o++] = ch == '@' ? tok_at_word : tok_id;
            tokens[o++] = tok_data_pos;
            *(int *)((intptr_t)tokens + o) = line;
            o += sizeof(int);
            *(int *)((intptr_t)tokens + o) = col;
            o += sizeof(int);
            tokens[o++] = ch;

            char ch = content[i];
            while ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || (ch >= 48 && ch <= 57) || ch == 95) {
                tokens[o++] = ch;
                ch = content[++i];
            }
            tokens[o++] = 0;
            continue;
        }
        // Number
        if (ch >= 48 && ch <= 57) {
            tokens[o++] = tok_number;
            tokens[o++] = ch;

            // 0-9
            char ch = content[i];
            while (ch >= 48 && ch <= 57) {
                tokens[o++] = ch;
                ch = content[++i];
            }
            tokens[o++] = 0;
            continue;
        }
        // Strings
        if (ch == '"') {
            int start = i;
            tokens[o++] = tok_string;

            char ch = content[i];
            while (ch != '"') {
                tokens[o++] = ch;
                if (ch == '\\') {
                    ch = content[++i];
                    tokens[o++] = ch;
                }
                if (ch == 0) {
                    lex_err(b, chunk, start, "Missing string closing tag '\"', compiler reached end of file");
                }
                ch = content[++i];
                // Extend memory if needed
                if ((o % 200 == 0) && str_buf->mem_size - o < 512) {
                    str_buf->length = o;
                    str_increase_memsize(str_buf, str_buf->mem_size * 2);
                    tokens = str_buf->data;
                }
            }
            tokens[o++] = 0;
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
                lex_err(b, chunk, i, "Missing character closing tag ('), found '%c'", content[i - 1]);
            }
            tokens[o++] = tok_char;
            tokens[o++] = ch;
            tokens[o++] = 0;
            continue;
        }
        // Scopes
        if (ch == '(' || ch == '[' || ch == '{' || (ch == '<' && content[i] == '{')) {
            tokens[o++] = bracket_token[ch];
            tokens[o++] = tok_data_scope_end;

            int index = o;
            o += sizeof(int);

            tokens[o++] = ch;
            if (ch == '<') {
                tokens[o++] = content[i++];
            }
            tokens[o++] = 0;

            closer_chars[depth] = bracket_table[ch];
            closer_indexes[depth] = index;
            depth++;
            continue;
        }
        if (ch == ')' || ch == ']' || ch == '}') {
            depth--;
            if (depth < 0) {
                lex_err(b, chunk, i, "Unexpected closing tag '%c'", ch);
            }
            if (closer_chars[depth] != ch) {
                lex_err(b, chunk, i, "Unexpected closing tag '%c', expected '%c'", ch, closer_chars[depth]);
            }
            tokens[o++] = bracket_token[ch];
            tokens[o++] = ch;
            tokens[o++] = 0;
            int offset = closer_indexes[depth];
            *(int *)(&tokens[offset]) = o;
            continue;
        }
        // =
        char op = -1;
        char next = content[i];
        int i_start = i - 1;

        if (ch == '=') {
            op = tok_eq;
            if(next == '=') {
                op = tok_eqeq;
                i++;
            } else if(next == '>') {
                op = tok_eqgt;
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
        } else if (ch == '~') {
            op = tok_tilde;
        } else if (ch == '?') {
            op = tok_qmark;
            if(next == '?') {
                op = tok_qqmark;
                i++;
            }
        } else if (ch == '^') {
            op = tok_bit_xor;
        } else if (ch == ':') {
            op = tok_colon;
        } else if (ch == '.') {
            op = tok_dot;
        } else if (ch == '#') {
            op = tok_hashtag;
        } else if (ch == ';') {
            op = tok_semi;
        } else if (ch == ',') {
            op = tok_comma;
        } else if (ch == '@') {
            op = tok_at;
        } else if (ch == '%') {
            op = tok_mod;
        }

        if(op > -1) {
            tokens[o++] = op;
            while(i_start < i) {
                tokens[o++] = content[i_start++];
            }
            tokens[o++] = 0;
            continue;
        }

        lex_err(b, chunk, i, "Unexpected character '%c' (byte:%d, pos:%d)", ch, ch, i);
    }

    if (depth > 0) {
        lex_err(b, chunk, i, "Missing closing tag '%c'", closer_chars[depth - 1]);
    }

    tokens[o++] = tok_eof;
    tokens[o++] = '\0';

    str_buf->length = o;
    chunk->tokens = str_to_chars(b->alc, str_buf);
    b->LOC += line;
}
