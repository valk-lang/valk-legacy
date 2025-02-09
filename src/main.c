
#include "all.h"

void help();

Array* watch_dirs;
bool is_watching;

int main(int argc, char *argv[]) {
    //
    if (argc < 2) {
        help();
    }

    //
    // lsp_doc_content = NULL;
    watch_dirs = NULL;
    is_watching = false;

    //
    char *cmd = argv[1];

    if (strcmp(cmd, "build") == 0) {
        cmd_build(argc, argv);
    } else {
        help();
    }
}

void help() {
    //
    printf("-------------------------\n");
    // printf(" 🦅 Valk v%s\n", VALK_VERSION);
    printf(" 🦅 Valk legacy compiler");
    printf("-------------------------\n\n");

    printf(" valk build -h       Build valk code to an executable\n");

    // printf("\n");
    // printf(" valk fmt -h         Format valk code\n");
    // printf(" valk ls -h          Run language server\n");

    printf("\n");
    exit(1);
}
