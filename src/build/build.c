
#include "../all.h"

void cmd_build_help();

int cmd_build(int argc, char *argv[]) {

    Allocator* alc = alc_make();
    char* char_buf = al(alc, 10 * 1024);

    // Parse args
    Map* options = map_make(alc);
    Array* args = array_make(alc, 20);
    Array* has_value = array_make(alc, 10);
    array_push(has_value, "-o");
    parse_argv(argv, argc, has_value, args, options);

    // Validate args
    char* main_dir = NULL;
    Array* vo_files = array_make(alc, 20);
    for(int i = 2; i < args->length; i++) {
        char* arg = array_get_index(args, i);
        if(ends_with(arg, ".vo")) {
            if(!file_exists(arg)) {
                sprintf(char_buf, "File not found: '%s'", arg);
                die(char_buf);
            }
            array_push(vo_files, arg);
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
        main_dir = arg;
    }
    if(!main_dir && vo_files->length == 0) {
        cmd_build_help();
        return 1;
    }

    // Build
    Build* b = al(alc, sizeof(Build));
    b->alc = alc;
    b->used_pkc_names = array_make(alc, 20);
    b->char_buf = char_buf;

    b->pkc_by_dir = map_make(alc);
    b->fc_by_path = map_make(alc);
    b->nsc_by_path = map_make(alc);

    b->func_main = NULL;

    b->export_count = 0;
    b->verbose = 3;
    b->LOC = 0;

    build_set_stages(b);

    Pkc* pkc_main = pkc_make(alc, b, "main");
    b->pkc_main = pkc_main;
    if (main_dir)
        pkc_set_dir(pkc_main, main_dir);

    Nsc* nsc_main = nsc_try_load(pkc_main, "main");
    if(!nsc_main) {
        nsc_main = nsc_make(alc, pkc_main, "main", pkc_main->dir);
    }
    b->nsc_main = nsc_main;

    // Build
    usize start = microtime();

    for(int i = 0; i < vo_files->length; i++) {
        char* path = array_get_index(vo_files, i);
        fc_make(nsc_main, path);
    }

    // Build stages
    build_run_stages(b);

    // Finish build
    printf("⌚ Lexer: %.3fs\n", (double)b->time_lex / 1000000);
    printf("⌚ Parse: %.3fs\n", (double)b->time_parse / 1000000);
    printf("⌚ Gen IR: %.3fs\n", (double)b->time_ir / 1000000);
    printf("⌚ LLVM: %.3fs\n", (double)b->time_llvm / 1000000);
    printf("⌚ Link: %.3fs\n", (double)b->time_link / 1000000);
    printf("⌚ File IO: %.3fs\n", (double)b->time_io / 1000000);
    printf("✅ Compiled in: %.3fs\n", (double)(microtime() - start) / 1000000);

    return 0;
}

void build_err(Build* b, char* msg) {
    printf("# Error: %s\n", msg);
    exit(1);
}
void parse_err(Chunk *chunk, char *msg) {
    printf("# Parse error\n");
    build_err(chunk->b, msg);
}

void cmd_build_help() {
    printf("\n# volt build {.vo-file|dir} [{more .vo-files}] -o {outpath}\n");

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
