#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <sys/wait.h>


#include "ptrace_utils.h"

unsigned long long find_library_base(const char *library, pid_t pid) {
    char maps_path[64];
    char line[1024];
    FILE *fp;
    unsigned long long addr = 0;

    if (pid == -1) {
        snprintf(maps_path, sizeof(maps_path), "/proc/self/maps");
    } else {
        snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    }

    fp = fopen(maps_path, "r");
    if (!fp) {
        perror("fopen maps");
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, library)) {
            addr = strtoull(line, NULL, 16);
            break;
        }
    }

    fclose(fp);
    return addr;
}

void *find_dlopen(pid_t target_pid) {
    void *local_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!local_dlopen) {
        fprintf(stderr, "[!] dlsym failed: %s\n", dlerror());
        return NULL;
    }

    unsigned long long local_libc = find_library_base("libc", -1);
    unsigned long long target_libc = find_library_base("libc", target_pid);

    if (!local_libc || !target_libc) {
        fprintf(stderr, "[!] Could not find libc base addresses\n");
        return NULL;
    }

    unsigned long long dlopen_offset = (unsigned long long)local_dlopen - local_libc;
    void *target_dlopen = (void *)(target_libc + dlopen_offset);

    printf("[*] dlopen() offset: 0x%llx\n", dlopen_offset);
    printf("[*] dlopen() in target: %p\n", target_dlopen);

    return target_dlopen;
}

int inject_library(pid_t pid, void *dlopen_addr, const char *library_path) {
    struct user_regs_struct oldregs, regs;
    int status;
    unsigned char *backup = NULL;
    unsigned long injection_addr;
    int ret = -1;

    if (!dlopen_addr || !library_path) {
        fprintf(stderr, "[!] Invalid parameters\n");
        return -1;
    }

    // Calcola dimensioni payload
    size_t path_len = strlen(library_path) + 1;
    size_t path_size = path_len; //align_to_word(path_len);
    size_t nop_size = 8;
    unsigned char stub[] = { 0xff, 0xd0, 0xcc };  // call rax; int3
    size_t stub_size = sizeof(stub);
    size_t total_size = path_size + nop_size + stub_size;

    // CRITICO: Allinea a 8 byte per ptrace
    //total_size = align_to_word(total_size);

    size_t path_offset = 0;
    size_t nop_offset = path_size;
    size_t stub_offset = path_size + nop_size;

    printf("[*] Starting injection for PID %d\n", pid);
    printf("[*] Library: %s\n", library_path);

    // Attach
    if (ptrace_attach(pid) < 0) {
        perror("Errore attach");
        return 1;
    }

    // Salva stato
    if (ptrace_getregs(pid, &oldregs) < 0) {
        perror("Errore lettura registri");
        ptrace_detach(pid);
        return 1;
    }
    memcpy(&regs, &oldregs, sizeof(regs));

    printf("[+] Original RIP: 0x%llx\n", oldregs.rip);

    injection_addr = regs.rip;

    // Backup codice originale
    backup = malloc(total_size);
    if (!backup) {
        perror("malloc");
        goto cleanup;
    }

    if (ptrace_read(pid, injection_addr, backup, total_size) < 0) {
        goto cleanup;
    }

    // Scrivi payload: path + nops + stub
    if (ptrace_write(pid, injection_addr + path_offset, library_path, path_len) == -1) {
        goto cleanup;
    }

    unsigned char nops[8] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    if (ptrace_write(pid, injection_addr + nop_offset,
                            nops, nop_size) == -1) {
        goto cleanup;
    }

    if (ptrace_write(pid, (unsigned long long)injection_addr + stub_offset,
                            stub, stub_size) == -1) {
        goto cleanup;
    }

    printf("[+] Payload written at RIP\n");

    // Setup registri per dlopen(library_path, RTLD_LAZY)
    regs.rip = (unsigned long long)injection_addr + stub_offset;
    regs.rax = (unsigned long long)dlopen_addr;
    regs.rdi = (unsigned long long)injection_addr + path_offset;
    regs.rsi = RTLD_LAZY;

    if (ptrace_setregs(pid, &regs) < 0) {
        goto cleanup;
    }

    // Esegui
    if (ptrace_cont(pid, 0) < 0) {
        goto cleanup;
    }

    if (waitpid(pid, &status, WUNTRACED) == -1) {
        goto cleanup;
    }

    // Loop per gestire stop intermedi (SIGCHLD da system(), etc)
    while (WIFSTOPPED(status)) {
        int sig = WSTOPSIG(status);

        if (sig == SIGTRAP) {
            // Nostro INT3!
            if (ptrace_getregs(pid, &regs) < 0) {
                goto cleanup;
            }

            if (regs.rax != 0x0) {
                printf("[+] Library injected at: %p\n", (void *)regs.rax);
                ret = 0;
            } else {
                printf("[-] Injection failed (dlopen returned NULL)\n");
            }

            // Ripristina codice
            if (ptrace_write(pid, injection_addr,
                                    backup, total_size) == -1) {
                goto cleanup;
            }

            // Restore dei registri
             if (ptrace_setregs(pid, &oldregs) == -1) {
                goto cleanup;
            }

            printf("[+] State restored\n");
            break;

        } else {
            // Stop intermedio, continua e passa il signal
            printf("[*] Intermediate stop signal: %d, continuing...\n", sig);
            if (ptrace_cont(pid, sig) == -1) {
                goto cleanup;
            }
            if (waitpid(pid, &status, WUNTRACED) == -1) {
                perror("waitpid");
                goto cleanup;
            }
        }
    }

    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "[-] Process exited unexpectedly\n");
    }

cleanup:
    if (backup) free(backup);
    ptrace_detach(pid);
    return ret;
}