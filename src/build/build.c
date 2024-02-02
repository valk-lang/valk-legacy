
#include "../all.h"

void cmd_build_help();

int cmd_build(int argc, char *argv[]) {

    Allocator* alc = alc_make();

    Build* b = al(alc, sizeof(Build));
    b->alc = alc;
    b->pkc_by_dir = map_make(alc);
    b->used_pkc_names = array_make(alc, 20);
    b->char_buf = al(alc, 10 * 1024);

    Pkc* pkc_main = pkc_make(alc, b, "main");
    pkc_main->is_main = true;
    // If args contains a dir : pkc_set_dir(pkc_main, dir)

    Nsc* nsc = nsc_try_load(pkc_main, "main");
    if(!nsc) {
        nsc = nsc_make(alc, pkc_main, "main", pkc_main->dir);
    }

    // foreach .vo files
    // fc_make(b, nsc)

    return 0;
}

void build_err(Build* b, char* msg) {
    printf("Error: %s\n", msg);
}
void parse_err(Build *b, Chunk *chunk, char *msg) {
    printf("Parse error:\n");
    build_err(b, msg);
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
