#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>


#include "../common/lib_inject_utils.h"


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <path_to_lib.so> <pid>\n", argv[0]);
        fprintf(stderr, "Example: %s /tmp/mylib.so 12345\n", argv[0]);
        return 1;
    }

    const char *lib_path = argv[1];
    pid_t target_pid = atoi(argv[2]);

    printf("lib_path: %s\n", lib_path);

    char abs_lib_path[PATH_MAX];
    if (realpath(lib_path, abs_lib_path) == NULL) {
        fprintf(stderr, "[!] Failed to resolve path '%s'\n", 
                lib_path);
        return 1;
    }

    printf("[*] Resolved path: %s\n", abs_lib_path);

    printf("[*] Absolute library path: %s\n", abs_lib_path);

    // Calcola dlopen() address
    void *dlopen_addr = find_dlopen(target_pid);
    if (!dlopen_addr) {
        return 1;
    }

    // Inject
    if (inject_library(target_pid, dlopen_addr, abs_lib_path) == 0) {
        printf("[+] Injection completed successfully\n");
        return 0;
    }
    fprintf(stderr, "[-] Injection failed\n");
    return 1;
}
