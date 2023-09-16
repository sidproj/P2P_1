#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(){
    int serverpipe1[2];//node.c read server.c write
    int serverpipe2[2];//server.c read node.c write

    int clientpipe1[2];//node.c read client.c write
    int clientpipe2[2];//server.c read node.c write

    //for server
    pipe(serverpipe1);
    pipe(serverpipe2);
    //for client
    pipe(clientpipe1);
    pipe(clientpipe2);

    if(fork()==0){//server child
        char spipe1[5];
        char spipe2[5];

        //interger into string
        sprintf(spipe1,"%d",serverpipe1[1]);//server.c write
        sprintf(spipe2,"%d",serverpipe2[0]);//server.c reads

        // char *arguments[] = {"./server.out",spipe1,spipe2,NULL};

        //starting server file in child process
        execl("./server.out","server.out",spipe1,spipe2,NULL);
        printf("Failed to start server child process\n");
        exit(1);
    }
    int t;
    printf("Do you want to start client?");
    scanf("%d",&t);
    if(fork()==0){//client child
        char cpipe1[5];
        char cpipe2[5];

        //interger into string
        sprintf(cpipe1,"%d",clientpipe1[1]);//client.c write
        sprintf(cpipe2,"%d",clientpipe2[0]);//client.c reads

        char *arguments[]={"./client.out",cpipe1,cpipe2,NULL};

        //starting client file in child process
        execl("./client.out","client.out",cpipe1,cpipe2,NULL);
        printf("Failed to start client child process\n");
        exit(1);
    }
    int statusServer,statusClient;
    pid_t pid1,pid2;
    pid1 = wait(&statusServer);
    pid2 = wait(&statusClient);
}