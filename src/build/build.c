
#include "../all.h"

void cmd_build_help();

int cmd_build(int argc, char *argv[]) {

    Allocator *alc = alc_make();
    char *char_buf = al(alc, 10 * 1024);
    Str *str_buf = str_make(alc, 100 * 1024);

    // Parse args
    Map *options = map_make(alc);
    Array *args = array_make(alc, 20);
    Array *has_value = array_make(alc, 10);
    array_push(has_value, "-o");
    parse_argv(argv, argc, has_value, args, options);

    // Validate args
    char *main_dir = NULL;
    Array *vo_files = array_make(alc, 20);
    for (int i = 2; i < args->length; i++) {
        char *arg = array_get_index(args, i);
        if (ends_with(arg, ".vo")) {
            if (!file_exists(arg)) {
                sprintf(char_buf, "File not found: '%s'", arg);
                die(char_buf);
            }
            char *fullpath = al(alc, VOLT_PATH_MAX);
            get_fullpath(arg, fullpath);
            array_push(vo_files, fullpath);
            continue;
        }
        if (!is_dir(arg)) {
            sprintf(char_buf, "Invalid file/directory: '%s'", arg);
            die(char_buf);
        }
        if (main_dir) {
            sprintf(char_buf, "You cannot pass 2 directories in the arguments: '%s' | '%s'", main_dir, arg);
            die(char_buf);
        }
        char *dir_buf = al(alc, VOLT_PATH_MAX);
        get_fullpath(arg, dir_buf);
        main_dir = dir_buf;
    }
    if (!main_dir && vo_files->length == 0) {
        cmd_build_help();
        return 1;
    }

    // Build
    Build *b = al(alc, sizeof(Build));
    b->alc = alc;
    b->alc_ast = alc_make();
    b->used_pkc_names = array_make(alc, 20);
    b->char_buf = char_buf;
    b->str_buf = str_buf;

    b->pkc_by_dir = map_make(alc);
    b->fc_by_path = map_make(alc);
    b->nsc_by_path = map_make(alc);

    b->func_main = NULL;

    b->ptr_size = 8;
    b->export_count = 0;
    b->verbose = 3;
    b->LOC = 0;

    build_set_stages(b);

    Pkc *pkc_main = pkc_make(alc, b, "main");
    b->pkc_main = pkc_main;
    if (main_dir)
        pkc_set_dir(pkc_main, main_dir);

    Nsc *nsc_main = nsc_load(pkc_main, "main", false);
    if (!nsc_main) {
        nsc_main = nsc_make(alc, pkc_main, "main", pkc_main->dir);
    }
    b->nsc_main = nsc_main;

    // Load core dependencies
    Pkc *vlt = pkc_load_pkc(pkc_main, "volt", NULL);
    Nsc *io = nsc_load(vlt, "io", true);
    Nsc *type = nsc_load(vlt, "type", true);
    b->pkc_volt = vlt;

    // Build
    usize start = microtime();

    for (int i = 0; i < vo_files->length; i++) {
        char *path = array_get_index(vo_files, i);
        fc_make(nsc_main, path);
    }

    // Build stages
    build_run_stages(b);
    // Link
    stage_5_objects(b);
    stage_6_link(b);

    // Finish build
    if (b->verbose > 0) {
        printf("📃 LOC: %d\n", b->LOC);
        printf("⌚ Lexer: %.3fs\n", (double)b->time_lex / 1000000);
        printf("⌚ Parse: %.3fs\n", (double)b->time_parse / 1000000);
        printf("⌚ Gen IR: %.3fs\n", (double)b->time_ir / 1000000);
        printf("⌚ LLVM: %.3fs\n", (double)b->time_llvm / 1000000);
        printf("⌚ Link: %.3fs\n", (double)b->time_link / 1000000);
        printf("⌚ File IO: %.3fs\n", (double)b->time_io / 1000000);
        printf("✅ Compiled in: %.3fs\n", (double)(microtime() - start) / 1000000);
    }

    return 0;
}

void build_err(Build *b, char *msg) {
    printf("# Error: %s\n", msg);
    exit(1);
}
void parse_err(Chunk *chunk, char *msg) {
    printf("# Parse error\n");
    Build *b = chunk->b;
    Array *chunks = array_make(b->alc, 10);
    Chunk *in = chunk;
    while (in) {
        array_push(chunks, in);
        in = in->parent;
    }
    if (chunks->length > 1) {
        int x = chunks->length;
        printf("------------------------------\n");
        while (--x >= 0) {
            Chunk *ch = array_get_index(chunks, x);
            printf("=> line: %d | col: %d | file: %s\n", ch->line, ch->col, ch->fc ? ch->fc->path : "(generated code)");
        }
        printf("------------------------------\n");
    }
    char *content = chunk->content;
    int line = chunk->line;
    int col = chunk->col;
    int col_end = chunk->col;
    int i = 0;
    chunk_lex(chunk, chunk->i, &i, &line, &col, &col_end);
    int len = col_end > col ? col_end - col : 1;
    if (len > 50)
        len = 50;

    printf("# File: %s\n", chunk->fc ? chunk->fc->path : "(generated code)");
    printf("# Line: %d | Col: %d\n", line, col);
    printf("# Error: %s\n", msg);
    int spaces = (col >= 10) ? 0 : (10 - col);
    i -= len + (10 - spaces) - 1;
    int x = len + 34;
    while (x-- > 0)
        printf("#");
    printf("\n");
    x = spaces;
    while (x-- > 0)
        printf(" ");
    x = 0;
    while (content[i] != 0 && content[i] != '\n' && x++ < (20 + len)) {
        char ch = content[i++];
        if(ch == '\t')
            ch = ' ';
        printf("%c", ch);
    }
    printf("\n######## ");
    x = len;
    while (x-- > 0)
        printf("^");
    printf(" ########################\n");
    exit(1);
}

void cmd_build_help() {
    printf("\n# volt build {.vo-file|config-dir} [{more .vo-files}] -o {outpath}\n");

    // printf(" --clean -c          clear cache\n");
    // printf(" --debug -d          generate debug info\n");
    // printf(" --optimize -O       apply code optimizations\n");
    // printf(" --run -r            run code after compiling\n");
    // printf(" --test              generate a 'main' that runs all tests\n");
    // printf(" --watch             watch files & rebuild when code changes\n");
    // printf("\n");

    // printf(" --def               define comptime variables\n");
    // printf("                     format: VAR1=VAL,VAR2=VAL\n");
    // printf(" --target            compile for a specific os/arch\n");
    // printf("                     linux-x64, macos-x64, win-x64\n");
    // printf(" -v -vv -vvv         show compile info\n");
    printf("\n");
}
