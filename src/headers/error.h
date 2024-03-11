
#ifndef _H_ERR
#define _H_ERR

#include "typedefs.h"

void build_err(Build *b, char *msg, ...);
void parse_err(Parser *p, int start, char *msg, ...);
void lex_err(Build* b, Chunk *ch, int content_i, char *msg, ...);
void get_content_line_col(char* content, int target_i, int* _line, int* _col);

#endif
