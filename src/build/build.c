
#include "../all.h"

void cmd_build_help();

int cmd_build(int argc, char *argv[]) {
}

void cmd_build_help() {
    printf("\n# volt build {.vo-files|dir} -o {outpath}\n");

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
