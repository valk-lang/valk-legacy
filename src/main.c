
#include "all.h"

void help();

int main(int argc, char *argv[]) {
    //
    if (argc < 2) {
        help();
    }

    //
    // lsp_doc_content = NULL;

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
    printf(" ⚡ Vali v0.0.1\n");
    printf("-------------------------\n\n");

    printf(" vali build -h       Build vali code to an executable\n");

    // printf("\n");
    // printf(" vali fmt -h         Format vali code\n");
    // printf(" vali ls -h          Run language server\n");

    printf("\n");
    exit(1);
}
