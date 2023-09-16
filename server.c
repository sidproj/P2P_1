#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <fcntl.h>

#define PORT 5000

char clients[5][15];
int top=1;
int readPipe;
int writePipe;
char ipaddr[15];

// structure to send and recv data
struct request{
    char type[1];
    char data[1024];
    char filename[20];
    int save;
};

struct clientIP{
    int index;
    char IP[15];
};

struct nodeList{
    char nodes[5][15];
};

struct IPC{
    char type[1];
    char data[1024];
    char IP[15];
    char filename[15];
    struct clientIP cip;
    struct nodeList nl;
};

struct manifest{
    char chunk_IP[5][2][15];
};

struct manifest MAN;


void getIP(){
    system("ifconfig | grep 'inet ' | sed -n '2 p' | awk '{print $2}' > serverip.txt");
    FILE * f = fopen("serverip.txt","r");
    fgets(ipaddr,15,f);
}

//function to devide a file into different chunks
void distributeFile(){
    // FILE *man = fopen("manifest","w");

    FILE *f = fopen("data.txt","r");

    int count;
    char c;
    
    for (c = getc(f); c != EOF; c = getc(f)) count = count + 1;
    
    fclose(f);
    int devide = (count / 4)+1;
    FILE * fd = fopen("data.txt","r");
    char msg[1024];
    for(int i=0; fgets(msg, devide, fd) != NULL ;i++){

        //get data from file
        if (i==3){
            memset(msg,0,1024);
            fgets(msg,devide,fd);
        }
        //create new file
        char filename[20];
        sprintf(filename,"chunk%d.p2p",i);
        FILE * nf = fopen(filename,"w");
        fprintf(nf,"%s",msg);

        //enter into manifest file
        strcpy(MAN.chunk_IP[i][0],filename);
        strcpy(MAN.chunk_IP[i][1],ipaddr);
        // fprintf(man,"%s \n",filename);

        fclose(nf);
        memset(msg,0,1024);
    }
    for(int i=0;i<5;i++){
        printf("filename :%s IP:%s\n",MAN.chunk_IP[i][0],
        MAN.chunk_IP[i][1]);
    }
}

//function that handles comunication with the client 
void communicate(int client,int pipeSend,int pipeRecv,int index){
    int cont=1;
    while(cont == 1){
        struct request req;
        memset(&req,0,sizeof(struct request));

        recv(client,&req,sizeof(struct request),0);
        //handle client's request and fire functions accordingly
        int con = (int)req.type[0];
        // printf("request: %d\n",req);
        if(con == 0) continue;
        cont = handleClientRequest(&req,client,pipeSend,pipeRecv,index);
        // distributeFile(clientIP);
    }
    printf("exited\n");
    exit(0);
}

int handleClientRequest(struct request * req,int client,
int pipeSend,int pipeRecv,int index){

    // printf("request : %s %d\n",req,req[0]);
    // if(strcmp(req,"CIP")==0){
    //     recvClientIP(client);
    //     return 1;
    // }
    switch(req->type[0]){
        case '1':recvClientIP(req,pipeSend,pipeRecv,index);break;
        case '2':getClientsIP(client,req,pipeSend,pipeRecv);break;
        case '3':sendManifestData(client);break;
        case '4':sendChunkFile(client,req);break;
        case '5':updateManifest(client,req,pipeSend,pipeRecv);break;
        case '6':printf("close connection.\n");return 0;
        default:printf("Invalid request from client.\n");
    }
    return 1;
}

void recvClientIP(struct request * req,int pipeSend,int pipeRecv,int index){
    //printing IP of the client
    printf("Connection established with client of IP %s",req->data);

    //struct for locking pipe descriptior
    struct flock fl;
    //setting propreties for lock
    fl.l_type = F_WRLCK;//setting write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    //-----------------------lock not working change code-----------------------
    //setting lock on write side of the pipe
    fcntl(pipeSend,F_SETLK,&fl);

    //sending signal to the parent
    kill(getppid(),SIGUSR1);
    
    //sending msg through pipe
    struct IPC ipc;
    memset(&ipc,0,sizeof(struct IPC));

    ipc.type[0]='1';

    ipc.cip.index = index;
    
    //copy ip
    strcpy(ipc.cip.IP,req->data);

    // ipc.cip = cip;

    write(pipeSend,&ipc,sizeof(struct IPC));

    // struct nodeList nl;
    // memset(&nl,0,sizeof(struct nodeList));
    // read(pipeRecv,&nl,sizeof(struct nodeList));


    
    fl.l_type = F_UNLCK;
    fcntl(pipeSend,F_SETLK,&fl);
    printf("Done\n");
}

void getClientsIP(int client,struct reuqest * req,int pipeSend,int pipeRecv){
    //struct for locking pipe descriptior
    struct flock fl;
    //setting propreties for lock
    fl.l_type = F_WRLCK;//setting write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    //-----------------------lock not working change code-----------------------
    //setting lock on write side of the pipe
    fcntl(pipeSend,F_SETLK,&fl);

    //sending signal to the parent
    kill(getppid(),SIGUSR1);
    
    //sending msg through pipe    
    
    struct IPC ipc;
    memset(&ipc,0,sizeof(struct IPC));

    ipc.type[0]='2';

    struct nodeList nl;
    memset(&nl,0,sizeof(struct nodeList));

    write(pipeSend,&ipc,sizeof(struct IPC));

    memset(&ipc,0,sizeof(struct IPC));
    
    read(pipeRecv,&nl,sizeof(struct nodeList));

    for(int i=0;i<5;i++){
        printf("IP [%d] : %s",i,nl.nodes[i]);
    }

    //unlock the pipe
    fl.l_type = F_UNLCK;
    fcntl(pipeSend,F_SETLK,&fl);
    send(client,&nl,sizeof(struct nodeList),0);
    printf("Done2\n");
}

void sendManifestData(int client){
    send(client,&MAN,sizeof(struct manifest),0);
}

void sendChunkFile(int client,struct request * req){
    
    struct request newReq;
    char data[1024];
    memset(&newReq,0,sizeof(struct request));

    FILE * f = fopen(req->filename,"r");

    fgets(data,1024,f);
    if(strncmp("chunk0.p2p",req->filename,10) == 0){
        newReq.save = 1;
        printf("====\nsave\n====\n");
    }
    strcpy(newReq.data,data);
    printf("data: %s\n",newReq.data);
    send(client,&newReq,sizeof(struct request),0);

    printf("Sent file\n");
    fclose(f);
}

void updateManifest(int client,struct request * req,int pipeSend,int pipeRecv){
    printf("Updating manifest for file: %s with IP: %s",req->filename,req->data);

    //struct for locking pipe descriptior
    struct flock fl;
    //setting propreties for lock
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    //-----------------------lock not working change code-----------------------
    //setting lock on write side of the pipe
    fcntl(pipeSend,F_SETLK,&fl);

    //sending signal to the parent
    kill(getppid(),SIGUSR1);

    struct IPC ipc;
    memset(&ipc,0,sizeof(struct IPC));
    ipc.type[0] = '3';
    strcpy(ipc.filename,req->filename);
    strcpy(ipc.IP,req->data);

    write(pipeSend,&ipc,sizeof(struct IPC));

    fl.l_type = F_UNLCK;
    fcntl(pipeSend,F_SETFL,&fl);
    printf("Manifest Updated\n");

}

//signal handler for IPC
void signalHandler(int sig){
    struct IPC ipc;
    memset(&ipc,0,sizeof(struct IPC));
    read(readPipe,&ipc,sizeof(struct IPC));

    switch(ipc.type[0]){
        case '1':recvIP(&ipc);break;
        case '2':sendIPS(&ipc);break;
        case '3':recvManifestData(&ipc);break;//save ip in manifest data
    }
}

//sub signal handler for IPC
void recvIP(struct IPC * ipc){
    printf("Called the signal: %s\n",ipc->cip.IP);
    //copy ip
    strcpy(clients[ipc->cip.index],ipc->cip.IP);
    displayClientIPS();
}

void sendIPS(struct IPC * ipc){
    printf("Called the signal\n");
    struct nodeList nl;
    memset(&nl,0,sizeof(struct nodeList));

    for(int i=0;i<5;i++){
        strcpy(nl.nodes[i],clients[i]);
    }
    write(writePipe,&nl,sizeof(struct nodeList));
}

void recvManifestData(struct IPC * ipc){
    printf("Called the signal\n");
    for(int i=0;i<5;i++){
        if( strcmp(MAN.chunk_IP[i][0],ipc->filename)==0){
            strcpy(MAN.chunk_IP[i][1],ipc->IP);
            break;
        }
    }
}

void displayClientIPS(){
    for(int i=0;i<5;i++){
        printf("client [%d] : %s\n",i,clients[i]);
    }
}

int main(){

    //for IPC
    int pipes1[2];//server.c read 'comunicate function' write
    int pipes2[2];//'comunicate function' read server.c write

    pipe(pipes1);
    pipe(pipes2);
    readPipe = pipes1[0];
    writePipe = pipes2[1];

    //for server program
    int sockfd,length;
    struct sockaddr_in address;

    //distribute file before starting server
    getIP();
    distributeFile();

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        printf("Error while creating socket\n");
        exit(0);
    }
    
    printf("Socket created successfully\n");
    
    //setting properties for the socket
    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    //binding socket
    
    if( (bind(sockfd,(struct sockaddr*)&address,sizeof(address)))!=0){
        printf("Error while binding socket.\n");
        exit(0);
    }

    printf("Binded socket\n");

    if((listen(sockfd,5))!=0){
        printf("Error while listening.\n");
        exit(0);
    }
    printf("Listening...\n");

    signal(SIGUSR1,signalHandler);
    while(1){
        for(int i=0;i<5;i++){
            length = sizeof(address);
            //accept new client connection
            int client = accept(sockfd,(struct sockaddr*)&address,&length);
            if(fork()==0){//after client is connected 
                
                communicate(client,pipes1[1],pipes2[0],i);
            }
            top++;
        }
    }
}