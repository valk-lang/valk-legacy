
#include "../all.h"

void cmd_build_help();

int cmd_build(int argc, char *argv[]) {

    size_t mem_start = get_mem_usage();

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
        if (arg[0] == '-') {
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
    char* path_out = map_get(options, "-o");
    if (!path_out) {
        cmd_build_help();
        return 1;
    }

    // Options
    bool is_test = array_contains(args, "--test", arr_find_str);

    // Build
    Build *b = al(alc, sizeof(Build));
    b->alc = alc;
    b->alc_ast = alc_make();
    b->used_pkc_names = array_make(alc, 20);
    b->char_buf = char_buf;
    b->str_buf = str_buf;
    b->path_out = path_out;

    b->os = "linux";
    b->arch = "x64";

    b->pkc_by_dir = map_make(alc);
    b->fc_by_path = map_make(alc);
    b->nsc_by_path = map_make(alc);
    b->pkcs = array_make(alc, 20);

    b->units = array_make(alc, 40);
    b->classes = array_make(alc, 40);
    b->pool_str = array_make(alc, 20);
    b->errors = al(alc, sizeof(ErrorCollection));
    b->errors->errors = map_make(alc);
    b->strings = array_make(alc, 100);

    b->func_main = NULL;
    b->func_main_gen = NULL;
    b->func_mark_globals = NULL;
    b->func_mark_shared = NULL;

    b->pkc_main = NULL;
    b->nsc_main = NULL;

    b->ptr_size = 8;
    b->error_count = 0;
    b->export_count = 0;
    b->string_count = 0;
    b->gc_vtables = 0;
    b->verbose = 2;
    b->LOC = 0;
    b->parser_started = false;

    b->is_test = is_test;

    // Cache dir
    char *cache_buf = al(alc, 1000);
    char *cache_hash = al(alc, 64);
    char *cache_dir = al(alc, VOLT_PATH_MAX);
    strcpy(cache_buf, main_dir ? main_dir : ".");
    strcat(cache_buf, "||");
    // strcat(cache_buf, os);
    // strcat(cache_buf, arch);
    // strcat(cache_buf, optimize ? "1" : "0");
    // strcat(cache_buf, debug ? "1" : "0");
    // strcat(cache_buf, test ? "1" : "0");
    ctxhash(cache_buf, cache_hash);
    strcpy(cache_dir, get_storage_path());
    strcat(cache_dir, "/cache/");
    if (!file_exists(cache_dir))
        makedir(cache_dir, 0700);
    strcat(cache_dir, cache_hash);
    strcat(cache_dir, "/");
    if (!file_exists(cache_dir))
        makedir(cache_dir, 0700);
    b->cache_dir = cache_dir;
    if (b->verbose > 0)
        printf("📦 Cache directory: %s\n", cache_dir);

    //
    build_set_stages(b);

    Pkc *pkc_main = pkc_make(alc, b, "main");
    b->pkc_main = pkc_main;
    if (main_dir)
        pkc_set_dir(pkc_main, main_dir);

    Nsc *nsc_main = nsc_load(pkc_main, "main", false, NULL);
    if (!nsc_main) {
        nsc_main = nsc_make(alc, pkc_main, "main", pkc_main->dir);
        map_set_force_new(pkc_main->namespaces, "main", nsc_main);
    }
    b->nsc_main = nsc_main;

    // Load core dependencies
    Pkc *vlt = pkc_load_pkc(pkc_main, "volt", NULL);
    Nsc *core = nsc_load(vlt, "core", true, NULL);
    Nsc *io = nsc_load(vlt, "io", true, NULL);
    Nsc *mem = nsc_load(vlt, "mem", true, NULL);
    Nsc *type = nsc_load(vlt, "type", true, NULL);
    b->pkc_volt = vlt;

    // Build
    usize start = microtime();

    for (int i = 0; i < vo_files->length; i++) {
        char *path = array_get_index(vo_files, i);
        fc_make(nsc_main, path);
    }

    // Build stages
    build_run_stages(b);
    if(b->func_main) {
        stage_4_ast_main(b->func_main->unit);
    }

    // Object files + Linking
    alc_delete(b->alc_ast);
    size_t mem_after_parse = get_mem_usage();
    stage_5_objects(b);

    // Finish build
    if (b->verbose > 0) {
        printf("📃 LOC: %d\n", b->LOC);
        printf("⌚ Lexer: %.3fs\n", (double)b->time_lex / 1000000);
        printf("⌚ Parse: %.3fs\n", (double)b->time_parse / 1000000);
        printf("⌚ Gen IR: %.3fs\n", (double)b->time_ir / 1000000);
        printf("⌚ LLVM: %.3fs\n", (double)b->time_llvm / 1000000);
        printf("⌚ Link: %.3fs\n", (double)b->time_link / 1000000);
        printf("⌚ File IO: %.3fs\n", (double)b->time_io / 1000000);
        if(b->mem_parse > 0) {
            printf("💾 Mem peak parser: %.2f MB\n", (double)(b->mem_parse) / (1024 * 1024));
        }
        if(b->mem_objects > 0) {
            printf("💾 Mem peak LLVM: %.2f MB\n", (double)(b->mem_objects - mem_after_parse) / (1024 * 1024));
        }
        printf("✅ Compiled in: %.3fs\n", (double)(microtime() - start) / 1000000);
    }

    return 0;
}

Str* build_get_str_buf(Build* b) {
    Str* res = b->pool_str->length > 0 ? array_pop(b->pool_str) : str_make(b->alc, 1000);
    str_clear(res);
    return res;
}
void build_return_str_buf(Build* b, Str* buf) {
    array_push(b->pool_str, buf);
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
