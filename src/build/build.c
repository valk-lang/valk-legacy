
#include "../all.h"

void cmd_build_help();
int default_arch();
int default_os();

void watch_files(Allocator* alc, bool autorun, Array* vo_files, char* path_out, int argc, char** argv);

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
    array_push(has_value, "--target");
    array_push(has_value, "--def");
    parse_argv(argv, argc, has_value, args, options);

    // Validate args
    char *main_dir = NULL;
    Array *vo_files = array_make(alc, 20);
    for (int i = 2; i < args->length; i++) {
        char *arg = array_get_index(args, i);
        if (ends_with(arg, ".va")) {
            if (!file_exists(arg)) {
                printf("File not found: '%s'\n", arg);
                return 1;
            }
            char *fullpath = al(alc, VALK_PATH_MAX);
            get_fullpath(arg, fullpath);
            array_push(vo_files, fullpath);
            continue;
        }
        if (arg[0] == '-') {
            continue;
        }
        if (!is_dir(arg)) {
            printf("Invalid file/directory: '%s'\n", arg);
            return 1;
        }
        if (main_dir) {
            printf(char_buf, "You cannot pass 2 directories in the arguments: '%s' | '%s'\n", main_dir, arg);
            return 1;
        }
        char *dir_buf = al(alc, VALK_PATH_MAX);
        get_fullpath(arg, dir_buf);
        main_dir = dir_buf;
    }
    if (vo_files->length == 0) {
        cmd_build_help();
        return 1;
    }

    if(array_contains(args, "--help", arr_find_str) || array_contains(args, "-h", arr_find_str)) {
        cmd_build_help();
        return 0;
    }

    // Options
    bool is_test = array_contains(args, "--test", arr_find_str) || array_contains(args, "-t", arr_find_str);
    bool autorun = array_contains(args, "--run", arr_find_str) || array_contains(args, "-r", arr_find_str);
    bool is_clean = array_contains(args, "--clean", arr_find_str) || array_contains(args, "-c", arr_find_str);
    bool optimize = !array_contains(args, "--no-opt", arr_find_str);

    char* path_out = map_get(options, "-o");
    if (!path_out && !autorun) {
        cmd_build_help();
        return 1;
    }

    int verbose = 0;
    if(array_contains(args, "-v", arr_find_str)) {
        verbose = 1;
    }
    if(array_contains(args, "-vv", arr_find_str)) {
        verbose = 2;
    }
    if(array_contains(args, "-vvv", arr_find_str)) {
        verbose = 3;
    }

    Map* cc_defs = build_cc_defs(alc, options);

    int os = default_os();
    int arch = default_arch();
    int target_os = os;
    int target_arch = arch;
    char *target = map_get(options, "--target");

    if (target) {
        if (strcmp(target, "linux-x64") == 0) {
            target_os = os_linux;
            target_arch = arch_x64;
        // } else if (strcmp(target, "linux-arm64") == 0) {
        //     target_os = os_linux;
        //     target_arch = arch_arm64;
        } else if (strcmp(target, "macos-x64") == 0) {
            target_os = os_macos;
            target_arch = arch_x64;
        } else if (strcmp(target, "macos-arm64") == 0) {
            target_os = os_macos;
            target_arch = arch_arm64;
        } else if (strcmp(target, "win-x64") == 0) {
            target_os = os_win;
            target_arch = arch_x64;
        // } else if (strcmp(target, "win-arm64") == 0) {
        //     target_os = os_win;
        //     target_arch = arch_arm64;
        } else {
            printf("Unsupported target: '%s'\nOptions: linux-x64, macos-x64, macos-arm64, win-x64\n", target);
            return 1;
        }
    }

    if(target_arch == arch_x86) {
        printf("x86-32bit is currently not supported by the compiler\n");
        return 1;
    }
    if(target_arch == arch_other) {
        printf("Your target architecture is unknown / not-supported by the compiler\n");
        return 1;
    }
    if(target_os == os_bsd) {
        printf("BSD is currently not supported by the compiler\n");
        return 1;
    }
    if(target_os == os_other) {
        printf("The compiler does not recognize the operating system you are using\n");
        return 1;
    }

    char* os_name = os_str(target_os);
    char* arch_name = arch_str(target_arch);
    map_set(cc_defs, "OS", os_name);
    map_set(cc_defs, "ARCH", arch_name);

    // Build
    Build *b = al(alc, sizeof(Build));
    b->alc = alc;
    b->alc_ast = alc_make();
    b->used_pkc_names = array_make(alc, 20);
    b->char_buf = char_buf;
    b->str_buf = str_buf;
    b->path_out = path_out;

    b->pkc_by_dir = map_make(alc);
    b->fc_by_path = map_make(alc);
    b->nsc_by_path = map_make(alc);
    b->pkcs = array_make(alc, 20);

    b->globals = array_make(alc, 40);

    b->units = array_make(alc, 100);
    b->classes = array_make(alc, 1000);
    b->pool_str = array_make(alc, 20);
    b->strings = array_make(alc, 100);
    b->links = array_make(alc, 20);
    b->link_settings = map_make(alc);
    b->parse_later = array_make(alc, 20);

    b->func_main = NULL;
    b->func_main_gen = NULL;

    b->pkc_main = NULL;
    b->nsc_main = NULL;

    b->cc_defs = cc_defs;
    b->host_arch = arch;
    b->target_arch = target_arch;
    b->host_os = os;
    b->target_os = target_os;

    b->ptr_size = 8;
    b->export_count = 0;
    b->string_count = 0;
    b->coro_count = 0;
    b->gc_vtables = 0;
    b->verbose = verbose;
    b->LOC = 0;
    b->parser_started = false;
    b->building_ast = true;
    b->parse_last = false;
    b->stage_1_done = false;

    b->is_test = is_test;
    b->is_clean = is_clean;
    b->optimize = optimize;

    // Cache dir
    char *cache_buf = al(alc, 1000);
    char *cache_hash = al(alc, 64);
    char *cache_dir = al(alc, VALK_PATH_MAX);
    strcpy(cache_buf, main_dir ? main_dir : ".");
    strcat(cache_buf, "||");
    strcat(cache_buf, os_name);
    strcat(cache_buf, arch_name);
    strcat(cache_buf, optimize ? "1" : "0");
    // strcat(cache_buf, debug ? "1" : "0");
    strcat(cache_buf, is_test ? "1" : "0");
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
    if (b->verbose > 0) {
        #ifdef WIN32
        printf("> Cache directory: %s\n", cache_dir);
        #else
        printf("📦 Cache directory: %s\n", cache_dir);
        #endif
    }

    // Generate out path if needed
    if(!b->path_out) {
        path_out = al(alc, VALK_PATH_MAX);
        strcpy(path_out, b->cache_dir);
        strcat(path_out, "tmp_build");
        b->path_out = path_out;
    }

    // Watch files
    if ((array_contains(args, "--watch", arr_find_str) || array_contains(args, "-w", arr_find_str)) && !is_watching) {
        watch_files(alc, autorun, vo_files, b->path_out, argc, argv);
        return 0;
    }

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
    nsc_main->unit->is_main = true;

    // Load core dependencies
    Pkc *vlt = pkc_load_pkc(pkc_main, "valk", NULL);
    Nsc *core = nsc_load(vlt, "core", true, NULL);
    Nsc *io = nsc_load(vlt, "io", true, NULL);
    Nsc *mem = nsc_load(vlt, "mem", true, NULL);
    Nsc *type = nsc_load(vlt, "type", true, NULL);
    b->pkc_valk = vlt;

    // Build
    usize start = microtime();

    loop(vo_files, i) {
        char *path = array_get_index(vo_files, i);
        fc_make(nsc_main, path, false);
    }

    // Build stages
    b->parser_started = true;
    build_run_stages(b);

    // Object files + Linking
    alc_delete(b->alc_ast);
    size_t mem_after_parse = get_mem_usage();
    stage_5_ir_final(b);
    stage_5_objects(b);

    // Finish build
    if (b->verbose > 0) {
        #ifdef WIN32
        printf("- LOC: %d\n", b->LOC);
        printf("- Lexer: %.3fs\n", (double)b->time_lex / 1000000);
        printf("- Parse: %.3fs\n", (double)b->time_parse / 1000000);
        printf("- Gen IR: %.3fs\n", (double)b->time_ir / 1000000);
        printf("- LLVM: %.3fs\n", (double)b->time_llvm / 1000000);
        printf("- Link: %.3fs\n", (double)b->time_link / 1000000);
        printf("- File IO: %.3fs\n", (double)b->time_io / 1000000);
        #else
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
        #endif
    }

    // Flush all output
#ifdef WIN32
    _flushall();
#else
    sync();
#endif
    int i = 0;
    while (!file_exists(b->path_out)) {
        sleep_ms(10);
        i++;
        if (i == 100)
            break;
    }

    if(autorun && !is_watching) {

        char cmd[VALK_PATH_MAX];
        strcpy(cmd, "\"");
        strcat(cmd, b->path_out);
        strcat(cmd, "\"");
        int code = system(cmd);
        code = code == 0 ? 0 : 1;
        exit(code);

    } else {
        #ifdef WIN32
        printf("> Compiled in: %.3fs\n", (double)(microtime() - start) / 1000000);
        #else
        printf("✅ Compiled in: %.3fs\n", (double)(microtime() - start) / 1000000);
        #endif
    }

    loop(b->pkcs, i) {
        Pkc* pkc = array_get_index(b->pkcs, i);
        if(pkc->config) {
            cJSON_Delete(pkc->config->json);
        }
    }

    alc_delete(b->alc);

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


Map* build_cc_defs(Allocator* alc, Map* options) {

    Map* res = map_make(alc);

    char* defs = map_get(options, "--def");
    if(!defs)
        return res;

    int len = strlen(defs);
    int i = 0;
    bool read_key = true;
    int part_i = 0;
    char part[256];
    char key[256];

    while (i < len) {
        char ch = defs[i];
        i++;
        if (ch == '=' && read_key) {
            part[part_i] = '\0';
            part_i = 0;
            if (!is_valid_varname_all(part)) {
                printf("Invalid comptime variable name: '%s'\n", part);
                exit(1);
            }
            strcpy(key, part);
            read_key = false;
            continue;
        }
        if (ch == ',' && !read_key) {
            part[part_i] = '\0';
            part_i = 0;

            map_set(res, dups(alc, key), dups(alc, part));

            read_key = true;
            continue;
        }
        part[part_i] = ch;
        part_i++;
    }
    if (!read_key) {
        part[part_i] = '\0';
        map_set(res, dups(alc, key), dups(alc, part));
    }

    return res;
}

int default_os() {
//
#if _WIN32
    return os_win;
#endif
#if __unix__
#if __linux__
    return os_linux;
#else
    return os_bsd;
#endif
#endif
#if __APPLE__
    return os_macos;
#endif
    return os_other;
}

int default_arch() {
//
#if defined(__x86_64__) || defined(_M_X64)
    return arch_x64;
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    return arch_x86;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return arch_arm64;
#endif
    return arch_other;
}


void watch_files(Allocator* alc, bool autorun, Array* vo_files, char* path_out, int argc, char** argv) {
    is_watching = true;
    bool first_run = true;
    watch_dirs = array_make(alc, 20);
    // Add directories from files args
    loop(vo_files, i) {
        char *file = array_get_index(vo_files, i);
        char md[VALK_PATH_MAX];
        get_dir_from_path(file, md);
        char *dir = dups(alc, md);
        array_push_unique_chars(watch_dirs, dir);
    }

    //
    Map *mod_times = map_make(alc);
    cmd_build(argc, argv);
    Allocator *walc = alc_make();
    int pid = -1;
    bool run_after_build = true;
    while (true) {
        alc_wipe(walc);
        bool build = false;
        loop(watch_dirs, i) {
            char *dir = array_get_index(watch_dirs, i);
            Array *files = get_subfiles(walc, dir, false, true);
            loop(files, o) {
                char *file = array_get_index(files, o);
                if (!ends_with(file, ".va")) {
                    continue;
                }
                int mt = mod_time(file);
                int pmt = map_get_i32(mod_times, file);
                if (mt != pmt) {
                    build = true;
                    map_set_i32(mod_times, file, mt);
                }
            }
        }

        if (!build && !first_run) {
            // Check autorun
            if (autorun && run_after_build) {
                run_after_build = false;
                // TODO : run program in different thread/process
                // Kill old process
                if (pid != -1) {
                }
                // Start new process
                char cmd[VALK_PATH_MAX];
                strcpy(cmd, "\"");
                strcat(cmd, path_out);
                strcat(cmd, "\"");
                int code = system(cmd);
                code = code == 0 ? 0 : 1;
            }
            sleep_ms(200);
            continue;
        }

        run_after_build = true;

        time_t timer = time(NULL);
        struct tm *tm_info = localtime(&timer);
        char buffer[26];
        strftime(buffer, 26, "%H:%M:%Ss", tm_info);

        if (first_run) {
            printf("[%s] Watching files\n", buffer);
            first_run = false;
            continue;
        }

        printf("[%s] Watcher: files changed -> build\n", buffer);
        cmd_build(argc, argv);
    }
}


void cmd_build_help() {
    printf("\n# valk build {.va-file(s)} [{config-dir}] -o {outpath}\n");
    printf("or\n");
    printf("# valk build {.va-file(s)} [{config-dir}] -r|--run\n\n");

    printf(" -o                  set outpath\n");
    printf(" --run -r            run program after compiling\n");
    printf(" --watch -w          watch files & rebuild when code changes\n");
    printf(" --test -t           build tests\n");
    printf(" --clean -c          ignore cache\n");
    // printf(" --debug -d          generate debug info\n");
    // printf(" --no-opt            build without optimizations\n");
    printf("\n");

    printf(" --def               define compile condition variables\n");
    printf("                     format: VAR1=VAL,VAR2=VAL\n");
    printf(" --target            compile for a specific os/arch\n");
    printf("                     linux-x64, macos-x64, win-x64\n");

    printf("\n");
    printf(" -v -vv -vvv         show compile info\n");
    printf(" --help -h           build command usage info\n");
    printf("\n");
}