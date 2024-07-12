
#include "../all.h"

#include <stdarg.h>

void error_print_code(Allocator*alc, char* content, int line, int col);

void build_err(Build *b, char *msg, ...) {

    va_list args;
    va_start(args, msg);
    char error[2048];
    vsprintf(error, msg, args);
    va_end(args);

    printf("# Error: %s\n", error);
    exit(1);
}

void parse_err(Parser *p, int start, char *msg, ...) {
    printf("# Parse error\n");
    Build *b = p->b;
    Chunk* chunk = p->chunk;
    char *content = chunk->content;

    if(!chunk->fc) {
        printf("########## CODE ##########\n");
        printf("%s", chunk->content);
        if(chunk->content[chunk->length - 1] != '\n') 
            printf("\n");
        printf("######## END CODE ########\n");
    }

    // Trace
    if (p->prev) {
        printf("------------------------------\n");
        Parser* sp = p->prev;
        while (sp) {
            Chunk* ch = sp->chunk;
            ChunkPos* pos = chunk_token_pos(b, ch, ch->i);
            printf("=> line: %d | col: %d | file: %s\n", pos->line, pos->col, ch->fc ? ch->fc->path : "(generated code)");
            sp = sp->prev;
        }
        printf("------------------------------\n");
    }
    // Position
    ChunkPos *pos = chunk_token_pos(b, chunk, chunk->i);
    ChunkPos *start_pos = pos;
    if (start > -1) {
        start_pos = chunk_token_pos(b, chunk, start);
    }
    int line = pos->line;
    int col = pos->col;

    va_list args;
    va_start(args, msg);
    char error[2048];
    vsprintf(error, msg, args);
    va_end(args);

    printf("# File: %s\n", chunk->fc ? chunk->fc->path : "(generated code)");
    printf("# Line: %d | Col: %d\n", line, col);
    printf("# Error: %s\n", error);

    error_print_code(b->alc, chunk->content, line, col);

    loop(b->pkcs, i) {
        Pkc* pkc = array_get_index(b->pkcs, i);
        if(pkc->config) {
            cJSON_Delete(pkc->config->json);
        }
    }

    alc_delete(b->alc_ast);
    alc_delete(b->alc);

    exit(1);
}

void error_print_code(Allocator*alc, char* content, int line, int col) {
    int i = 0;
    int len = strlen(content);
    int l = 1;
    int start_pos = 0;
    while(l < line && i < len) {
        char ch = content[i++];
        if(ch == '\n') {
            l++;
            start_pos = i;
        }
    }
    if(i == len)
        return;
    
    // Barriers
    // char* b1 = "###############################";
    char* b1 = "-------------------------------------";
    char* b2 = dups(alc, b1);
    int bl = strlen(b2);
    if (col > 0)
        col--;
    int offset = 0;
    if(col + 10 >= bl) {
        offset = col - bl + 10;
        col -= offset;
    }
    if (col > 0)
        b2[col - 1] = ' ';
    b2[col] = '^';
    b2[col + 1] = ' ';
    //

    // Get line start/end position
    start_pos += offset;
    i = start_pos;
    int end_pos = start_pos;
    while(i < len) {
        char ch = content[i++];
        if(ch == '\n')
            break;
        end_pos = i;
    }
    int line_len = end_pos - start_pos;

    // Print all
    printf("%s\n", b1);
    printf("%*.*s\n", line_len, line_len, content + start_pos);
    printf("%s\n", b2);
}

void lex_err(Build* b, Chunk *ch, int content_i, char *msg, ...) {

    char* content = ch->content;

    va_list args;
    va_start(args, msg);
    char error[2048];
    vsprintf(error, msg, args);
    va_end(args);

    int line, col;
    get_content_line_col(content, content_i, &line, &col);

    printf("# SYNTAX ERROR\n");
    printf("# File: %s\n", ch->fc ? ch->fc->path : "(generated code)");
    printf("# Line: %d | Col: %d\n", line, col);
    printf("# Error: %s\n", error);

    exit(1);
}


void get_content_line_col(char* content, int target_i, int* _line, int* _col) {
    int i = 0;
    int line = 1;
    int col = 0;
    while (i < target_i) {
        char ch = content[i++];
        if(ch == 0)
            break;
        col++;
        if(ch == '\n'){
            col = 0;
            line++;
        }
    }
    *_line = line;
    *_col = col;
}
