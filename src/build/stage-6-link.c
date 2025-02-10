
#include "../all.h"

Array* get_link_dirs(Build* b);
void stage_link_libs_all(Str *cmd, Build *b);

void stage_6_link(Build* b, Array* o_files) {

    if (b->verbose > 2)
        printf("Stage 6 | Link .o files to executable\n");

    usize start = microtime();

    if (b->verbose > 2)
        printf("Stage 6.1\n");

    Str *cmd = str_make(b->alc, 1000);

    bool is_linux = b->target_os == os_linux;
    bool is_macos = b->target_os == os_macos;
    bool is_win = b->target_os == os_win;

    bool is_x64 = b->target_arch == arch_x64;
    bool is_arm64 = b->target_arch == arch_arm64;

    bool host_os_is_target = b->host_os == b->target_os;
    bool host_arch_is_target = b->host_arch == b->target_arch;
    bool host_system_is_target = host_os_is_target && host_arch_is_target;

    char *linker = NULL;
    char linker_buf[VALK_PATH_MAX];
    if (host_system_is_target) {
        if (is_linux) {
            linker = "ld";
        } else if (is_macos) {
            linker = "ld";
        } else if (is_win) {
#ifdef __MINGW32__
            linker = "lld-link";
#else
            sprintf(linker_buf, "%s\\lld-link.exe", get_binary_dir());
            linker = linker_buf;
#endif
        }
    } else {
        if (is_linux) {
            linker = "ld.lld";
        } else if (is_macos) {
            linker = "ld64.lld";
        } else if (is_win) {
            linker = "lld-link";
        }
    }

    if (!linker) {
        build_err(b, "❌ Could not figure out which linker to use for your host os / target os.");
    }

    if (b->verbose > 2)
        printf("Stage 6.2 | Linker: %s\n", linker);

    //
    char *valk_lib_dir = b->pkc_valk->dir;

    str_append_chars(cmd, linker);
    str_append_chars(cmd, " ");
    if (is_win) {
        str_append_chars(cmd, "/out:\"");
    } else {
        str_append_chars(cmd, "-pie ");
        str_append_chars(cmd, "-o \"");
    }
    str_append_chars(cmd, b->path_out);
    // if (b->type == build_t_exe) {
        if (is_win && !ends_with(b->path_out, ".exe")) {
            str_append_chars(cmd, ".exe");
        }
    // } else if (b->type == build_t_shared_lib) {
    //     if (is_win && !ends_with(b->path_out, ".dll")) {
    //         str_append_chars(cmd, ".dll");
    //     } else if (is_linux && !ends_with(b->path_out, ".so")) {
    //         str_append_chars(cmd, ".so");
    //     } else if (is_macos && !ends_with(b->path_out, ".dylib")) {
    //         str_append_chars(cmd, ".dylib");
    //     }
    // }
    str_append_chars(cmd, "\" ");

    if (b->verbose > 2)
        printf("Stage 6.3 | Set link dirs\n");

    // Link dirs
    Array *link_dirs = get_link_dirs(b);
    loop(link_dirs, i) {
        char *path = array_get_index(link_dirs, i);
        str_append_chars(cmd, is_win ? "/libpath:\"" : "-L\"");
        str_append_chars(cmd, path);
        str_append_chars(cmd, "\" ");
    }

    if (b->verbose > 2)
        printf("Stage 6.4 | OS based link arguments\n");

    // Details
    if (is_linux) {
        str_append_chars(cmd, "--sysroot=/ ");
        // str_append_chars(cmd, valk_lib_dir);
        // str_append_chars(cmd, "root ");

        if (is_x64) {
            str_append_chars(cmd, "-m elf_x86_64 ");
            str_append_chars(cmd, "--dynamic-linker /lib64/ld-linux-x86-64.so.2 ");
        } else if (is_arm64) {
            str_append_chars(cmd, "-m aarch64linux ");
            str_append_chars(cmd, "--dynamic-linker /lib/ld-linux-aarch64.so.1 ");
        }

        // if (b->type == build_t_exe) {
            str_append_chars(cmd, "-l:Scrt1.o ");
            str_append_chars(cmd, "-l:crti.o ");
            str_append_chars(cmd, "-l:crtbeginS.o ");
        // } else if (b->type == build_t_shared_lib) {
        //     str_append_chars(cmd, "--shared ");
        // }

    } else if (is_macos) {
        str_append_chars(cmd, "-syslibroot / ");
        // str_append_chars(cmd, valk_lib_dir);
        // str_append_chars(cmd, "root ");

        // ppc, ppc64, i386, x86_64
        if (is_x64) {
            str_append_chars(cmd, "-arch x86_64 ");
        } else if (is_arm64) {
            str_append_chars(cmd, "-arch arm64 ");
        }
        // Set macos-version-min to 255.0 so we dont get warnings when linking
        str_append_chars(cmd, "-platform_version macos 255.0 11.1 ");
        // str_append_chars(cmd, "-sdk_version 11.1 ");
        // -macosx_version_min 11.1.0 -sdk_version 11.1.0

        // if (b->type == build_t_shared_lib) {
        //     str_append_chars(cmd, "-dylib ");
        // }
    } else if (is_win) {
        // /winsysroot:<value>
        str_append_chars(cmd, "/nodefaultlib /guard:ehcont ");
        // str_append_chars(cmd, "/force:unresolved ");
        if (is_x64) {
            str_append_chars(cmd, "/machine:x64 ");
        }
        // if (b->type == build_t_shared_lib) {
        //     str_append_chars(cmd, "/dll ");
        // }
    }

    if (b->verbose > 2)
        printf("Stage 6.5 | Set object files\n");

    // Object files
    loop(o_files, i) {
        char *path = array_get_index(o_files, i);
        str_append_chars(cmd, path);
        str_append_chars(cmd, " ");
    }

    if (b->verbose > 2)
        printf("Stage 6.6 | Link libraries\n");

    // Link libs
    stage_link_libs_all(cmd, b);

    if (b->verbose > 2)
        printf("Stage 6.7 | Finalize command\n");

    // End
    if (is_linux) {
        str_append_chars(cmd, "-l:crtendS.o ");
        str_append_chars(cmd, "-l:crtn.o ");
    }

    // Run command
    char *cmd_str = str_to_chars(b->alc, cmd);
    if (b->verbose > 1) {
        printf("Link cmd: %s\n", cmd_str);
    }
    int res = system(cmd_str);
    if (res != 0) {
        build_err(b, "❌ Failed to link");
    }

    b->time_link += microtime() - start;
}

void stage_link_libs_all(Str *cmd, Build *b) {
    //
    bool is_win = b->target_os == os_win;
    bool is_macos = b->target_os == os_macos;

    loop(b->links, i) {
        Link *link = array_get_index(b->links, i);

        char *prefix = "";
        char *suffix = "";
        bool is_static = link->type == link_static;
        if (!is_win) {
            prefix = "-l";
            if (is_static && !is_macos) {
                prefix = "-l:lib";
                suffix = ".a";
            }
        } else {
            suffix = ".lib";
        }

        str_append_chars(cmd, prefix);
        str_append_chars(cmd, link->name);
        str_append_chars(cmd, suffix);
        str_append_chars(cmd, " ");
    }
}

Array* get_link_dirs(Build* b) {
    Array* list = array_make(b->alc, 20);
    Array* pkcs = b->pkcs;
    loop(pkcs, i) {
        Pkc* pkc = array_get_index(pkcs, i);
        PkgConfig* cfg = pkc->config;
        if(!cfg)
            continue;

        cJSON *link = cJSON_GetObjectItemCaseSensitive(cfg->json, "link");
        if (link) {
            char fullpath[VALK_PATH_MAX];
            char target[256];
            char* os_name = os_str(b->target_os);
            char* arch_name = arch_str(b->target_arch);
            strcpy(target, os_name);
            strcat(target, "-");
            strcat(target, arch_name);

            cJSON *dirs = cJSON_GetObjectItemCaseSensitive(link, "dirs");
            if (dirs) {
                cJSON *cdir = dirs->child;
                while (cdir) {

                    char* dir = cdir->valuestring;
                    if (dir[0] == '/' || (dir[0] != 0 && dir[1] == ':')) {
                        strcpy(fullpath, "");
                    } else {
                        strcpy(fullpath, pkc->dir);
                    }
                    strcat(fullpath, dir);
                    fix_slashes(fullpath, true);
                    strcat(fullpath, target);
                    strcat(fullpath, "/");

                    if (!file_exists(fullpath)) {
                        printf("Link directory not found: %s => (%s)\n", cdir->valuestring, fullpath);
                        exit(1);
                    }

                    if (!array_contains(list, fullpath, arr_find_str)) {
                        array_push(list, dups(b->alc, fullpath));
                    }

                    cdir = cdir->next;
                }
            }

            cJSON *trg = cJSON_GetObjectItemCaseSensitive(link, "target");
            if (trg) {
                cJSON *os = cJSON_GetObjectItemCaseSensitive(trg, target);
                if (os) {
                    cJSON *dirs = cJSON_GetObjectItemCaseSensitive(os, "dirs");
                    if (dirs) {
                        char fullpath[VALK_PATH_MAX];
                        cJSON *cdir = dirs->child;
                        while (cdir) {

                            char* dir = cdir->valuestring;
                            if (dir[0] == '/' || (dir[0] != 0 && dir[1] == ':')) {
                                strcpy(fullpath, "");
                            } else {
                                strcpy(fullpath, pkc->dir);
                            }
                            strcat(fullpath, dir);
                            fix_slashes(fullpath, true);

                            if (!file_exists(fullpath)) {
                                continue;
                            }

                            if (!array_contains(list, fullpath, arr_find_str)) {
                                array_push(list, dups(b->alc, fullpath));
                            }

                            cdir = cdir->next;
                        }
                    }
                }
            }
        }
    }

    return list;
}
