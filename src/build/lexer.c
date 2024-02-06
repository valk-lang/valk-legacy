
#include "../all.h"

void chunk_lex_start(Chunk *chunk) {
    unsigned long start = microtime();
    chunk_lex(chunk, -1, NULL, NULL, NULL, NULL);
    chunk->b->time_lex += microtime() - start;
}

void chunk_lex(Chunk *chunk, int err_token_i, int *err_content_i, int *err_line, int *err_col, int *err_col_end) {
    //
    char *content = chunk->content;
    int length = chunk->length;
    Fc *fc = chunk->fc;
    Build *b = fc->b;

    if (fc && b->verbose > 2 && err_token_i == -1) {
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

    char token[256];
    int token_i = 0;

    int line = 1;
    int col = 0;
    int i_last = 0;

    Str *tokens_str = str_make(chunk->alc, length * 3 + 1024);
    char *tokens = tokens_str->data;

    while (true) {
        const char ch = content[i];
        if (err_token_i > -1 && o >= err_token_i) {
            *err_content_i = i;
            *err_line = line;
            *err_col = col;
            *err_col_end = col + (i + 1 - i_last);
            return;
        }
        if (ch == '\0')
            break;
        i++;
        col += i - i_last;
        i_last = i;
        // Make sure we have enough memory
        if (tokens_str->mem_size - o < 512) {
            tokens_str->length = o;
            str_increase_memsize(tokens_str, tokens_str->mem_size * 2);
            tokens = tokens_str->data;
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
                    ch = content[++i];
                    while (ch != '\n' && ch != 0) {
                        ch = content[++i];
                    }
                }
                if (ch == 0)
                    break;
                if (ch <= 32)
                    continue;
                break;
            }
            i--;
            tokens[o++] = has_newline ? tok_newline : tok_space;
            tokens[o++] = 0;
            continue;
        }

        // Compile conditions
        if (ch == '#' && (o < 2 || tokens[o - 2] == tok_newline)) {
            int x = i;
            char ch = content[x];
            while (ch >= 97 && ch <= 122) {
                token[token_i++] = ch;
                ch = content[++x];
            }
            token[token_i] = '\0';
            token_i = 0;
            if (str_is(token, "if")) {
                tokens[o++] = tok_cc;
                strcpy((char *)((intptr_t)tokens + o), token);
                o += 3;
                cc_depth++;
                i = x;
                continue;
            }
            if (str_is(token, "elif") || str_is(token, "else") || str_is(token, "end")) {
                tokens[o++] = tok_cc;
                strcpy((char *)((intptr_t)tokens + o), token);
                o += (str_is(token, "end")) ? 4 : 5;
                if (str_is(token, "end")) {
                    cc_depth--;
                    if (cc_depth < 0) {
                        chunk->i = i;
                        sprintf(b->char_buf, "Using #%s without an #if before it", token);
                        parse_err(chunk, b->char_buf);
                    }
                }
                i = x;
                continue;
            }
        }
        // ID: a-zA-Z_
        if ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || ch == 95 || ch == '@') {
            if (ch == '@') {
                tokens[o++] = tok_at_word;
            } else {
                tokens[o++] = tok_pos;
                *(int *)((intptr_t)tokens + o) = line;
                o += 4;
                *(int *)((intptr_t)tokens + o) = col;
                o += 4;
                tokens[o++] = tok_id;
            }
            tokens[o++] = ch;
            // a-zA-Z0-9_
            char ch = content[i];
            while ((ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || (ch >= 48 && ch <= 57) || ch == 95) {
                tokens[o++] = ch;
                ch = content[++i];
            }
            tokens[o++] = '\0';
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
            tokens[o++] = '\0';
            continue;
        }
        // Strings
        if (ch == '"') {
            tokens[o++] = tok_string;
            char ch = content[i];
            while (ch != '"') {
                tokens[o++] = ch;
                if (ch == '\\') {
                    ch = content[++i];
                    tokens[o++] = ch;
                }
                if (ch == 0) {
                    chunk->i = i;
                    sprintf(b->char_buf, "Missing string closing tag '\"', compiler reached end of file");
                    parse_err(chunk, b->char_buf);
                }
                ch = content[++i];
                // Extend memory if needed
                if ((o % 200 == 0) && tokens_str->mem_size - o < 512) {
                    tokens_str->length = o;
                    str_increase_memsize(tokens_str, tokens_str->mem_size * 2);
                    tokens = tokens_str->data;
                }
            }
            tokens[o++] = '\0';
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
                chunk->i = i;
                sprintf(b->char_buf, "Missing character closing tag ('), found '%c'", content[i - 1]);
                parse_err(chunk, b->char_buf);
            }
            tokens[o++] = tok_char_string;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            continue;
        }
        // Scopes
        if (ch == '(' || ch == '[' || ch == '{' || (ch == '<' && content[i] == '{')) {
            tokens[o++] = tok_scope_open;
            int index = o;
            o += sizeof(int);
            tokens[o++] = ch;
            if (ch == '<') {
                tokens[o++] = content[i++];
            }
            tokens[o++] = '\0';
            closer_chars[depth] = bracket_table[ch];
            closer_indexes[depth] = index;
            depth++;
            continue;
        }
        if (ch == ')' || ch == ']' || ch == '}') {
            depth--;
            if (depth < 0) {
                chunk->i = i;
                sprintf(b->char_buf, "Unexpected closing tag '%c'", ch);
                parse_err(chunk, b->char_buf);
            }
            if (closer_chars[depth] != ch) {
                chunk->i = i;
                sprintf(b->char_buf, "Unexpected closing tag '%c', expected '%c'", ch, closer_chars[depth]);
                parse_err(chunk, b->char_buf);
            }
            tokens[o++] = tok_scope_close;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            int offset = closer_indexes[depth];
            *(int *)(&tokens[offset]) = o;
            continue;
        }
        // Operators
        bool op2 = false;
        if (ch == '=' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '!' || ch == '<' || ch == '>') {
            const char ch2 = content[i];
            if (ch2 == '=')
                op2 = true;
            else if ((ch == '+' && ch2 == '+') || (ch == '-' && ch2 == '-') || (ch == '!' && ch2 == '!') || (ch == '!' && ch2 == '?') || (ch == '>' && ch2 == '>') || (ch == '<' && ch2 == '<')) {
                if (ch == '-' && content[i + 1] == '-') {
                    tokens[o++] = tok_op3;
                    tokens[o++] = ch;
                    tokens[o++] = ch;
                    tokens[o++] = ch;
                    tokens[o++] = '\0';
                    i += 2;
                    continue;
                }
                op2 = true;
            }
        } else if (ch == '?') {
            const char ch2 = content[i];
            if (ch2 == '?')
                op2 = true;
            else if (ch2 == '!')
                op2 = true;
        } else if (ch == '&' && content[i] == '&') {
            op2 = true;
        } else if (ch == '|' && content[i] == '|') {
            op2 = true;
        }
        if (op2) {
            tokens[o++] = tok_op2;
            tokens[o++] = ch;
            tokens[o++] = content[i];
            tokens[o++] = '\0';
            i++;
            continue;
        }
        if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '<' || ch == '>' || ch == '%' || ch == '^' || ch == '|' || ch == '=') {
            tokens[o++] = tok_op1;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            continue;
        }
        if (ch == '!' || ch == '&' || ch == ':' || ch == '?' || ch == '.' || ch == '~' || ch == '#' || ch == ';' || ch == ',') {
            tokens[o++] = tok_char;
            tokens[o++] = ch;
            tokens[o++] = '\0';
            continue;
        }

        chunk->i = i;
        sprintf(b->char_buf, "Unexpected character '%c' (byte:%d, pos:%d)", ch, ch, i);
        parse_err(chunk, b->char_buf);
    }

    if (depth > 0) {
        chunk->i = i;
        sprintf(b->char_buf, "Missing closing tag '%c'", closer_chars[depth - 1]);
        parse_err(chunk, b->char_buf);
    }

    tokens[o++] = tok_eof;
    tokens[o++] = '\0';

    chunk->tokens = tokens;
    b->LOC += line;

    // Probably will never happen
    // if (err_token_i > -1) {
    //     *err_content_i = i;
    //     *err_line = line;
    //     *err_col = col;
    //     return;
    // }
}
