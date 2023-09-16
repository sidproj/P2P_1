#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <wait.h>
#include <signal.h>
#define PORT 5000

char serverIP[15] = "10.200.2.40";
char ipaddr[15];
int readPipe;
int writePipe;

// structure to send and recv data
struct request{
    char type[1];
    char data[1024];
    char filename[20];
    int save;
};

struct nodeList{
    char nodes[5][15];
};

struct IPC{
    char type[1];
    char data[1024];
    struct nodeList nl;
    int childPID;
};

struct manifest{
    char chunk_IP[5][2][15];
};


void getIP(){
    system("ifconfig | grep 'inet ' | sed -n '2 p' | awk '{print $2}' > ip.txt");
    FILE * f = fopen("ip.txt","r");
    fgets(ipaddr,15,f);
}

void communicate(int sockfd,int pipeSend,int pipeRecv){
    //initially send client ip
    sendIP(sockfd);
    printf("Sent my IP\n");
    int i;
    scanf("%d",&i);
    // getValidNodes(sockfd,pipeSend,pipeRecv);
    // printf("Got Node's ips\n");
    struct manifest man;
    memset(&man,0,sizeof(struct manifest));
    getManifestData(sockfd,&man);

    for(int i=0;i<5;i++){
        printf("filename :%s IP:%s\n",man.chunk_IP[i][0],
        man.chunk_IP[i][1]);
    }

    scanf("%d",&i);
    getChunkFiles(sockfd,&man);
    
    //to keep client alive
    // int i;
    // scanf("%d",&i);

}

void sendIP(int sockfd){
    struct request req;//structure to send request
    //clearing variable of garbage values
    memset(&req,0,sizeof(struct request));
    //setting req for server to accept IP address
    req.type[0]='1';
    //copy ip address to structure
    strcpy(req.data,ipaddr);

    send(sockfd,&req,sizeof(struct request),0);
}

//get valid nodes from server to connect to
void getValidNodes(int sockfd,int pipeSend,int pipeRecv){
    struct request req;
    struct nodeList nl;

    memset(&nl,0,sizeof(nl));
    memset(&req,0,sizeof(struct request));
    
    req.type[0]='2';
    
    send(sockfd,&req,sizeof(struct request),0);
    recv(sockfd,&nl,sizeof(struct nodeList),0);

    for(int i=0;i<5;i++){
        printf("IP [%d] : %s\n",i,nl.nodes[i]);
    }
    sendNodeListToParent(&nl,pipeSend,pipeRecv);
}

//get manifest data from the server
void getManifestData(int sockfd,struct manifest * man){
    struct request req;
    memset(&req,0,sizeof(struct request));

    req.type[0]='3';

    send(sockfd,&req,sizeof(struct request),0);
    recv(sockfd,man,sizeof(struct manifest),0);
}

void getChunkFiles(int sockfd,struct manifest * man){
    int count=0;
    int status;
    for(int i=0;i<5;i++){
        if(strlen(man->chunk_IP[i][1])==0)break; 
        count++;
        if((fork())==0){
            printf("trying to get file:%s\n",man->chunk_IP[i][0]);
            clientSegmentForChunk(man->chunk_IP[i][1],man->chunk_IP[i][0]);
        }
        wait(&status);
    }
    // for(int i=0;i<count;i++){
    //     wait(&status);
    // }
}

void sendNodeListToParent(struct nodeList *nl,int pipeSend,int pipeRecv){
    kill(getppid(),SIGUSR1);
    struct IPC ipc;
    ipc.type[0]='1';

    for(int i=0;i<5;i++){
        strcpy(ipc.nl.nodes[i],nl->nodes[i]);
    }

    write(pipeSend,&ipc,sizeof(struct IPC));
    exit(0);
}

void clientSegmentForChunk(char * serverIP,char * filename){
    int sockfd;
    struct sockaddr_in address;
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        printf("Error while creating socket\n");
        exit(0);
    }
    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = inet_addr(serverIP);

    if((connect(sockfd,(struct sockaddr*)&address,sizeof(address)))!=0){
        printf("Connection with server failed.\n");
        exit(0);
    }
    printf("Connection with server established\n");
    
    //create request struct
    struct request req,req1;
    memset(&req,0,sizeof(struct request));
    req.type[0]='4';

    strcpy(req.filename,filename);
    
    send(sockfd,&req,sizeof(struct request),0);

    memset(&req1,0,sizeof(struct request));
    recv(sockfd,&req1,sizeof(struct request),0);
    
    //append in file
    FILE * f = fopen("newData.txt","a");
    printf("-----------\ndata from %s : %s\n---------\n",filename,req1.data);
    // fprintf("%s",req1.data);
    fprintf(f,"%s",req1.data);
    fclose(f);

    // check for save
    if(req1.save==1){
        char * temp = "test.p2p";
        FILE * chunk = fopen(temp,"w");
        fprintf(chunk,"%s",req1.data);
        // printf("%s",req1.data);
        fclose(chunk);
        
        //-----------------------------------------------------
        //sending request to update manifest file
        struct request manReq;
        memset(&manReq,0,sizeof(struct request));

        manReq.type[0]='5';
        strcpy(manReq.filename,filename);
        strcpy(manReq.data,ipaddr);
        printf("sent request to update manifest data\n");
        send(sockfd,&manReq,sizeof(struct request),0);
        //-----------------------------------------------------
    }
    exit(0);
}

void clientSegment(char * serverIP,int pipeSend,int pipeRecv){
    int sockfd;
    struct sockaddr_in address;
    
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        printf("Error while creating socket\n");
        exit(0);
    }
    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = inet_addr(serverIP);

    if((connect(sockfd,(struct sockaddr*)&address,sizeof(address)))!=0){
        printf("Connection with server failed.\n");
        exit(0);
    }
    printf("Connection with server established\n");
    communicate(sockfd,pipeSend,pipeRecv);
}

void signalHandler(int sig){
    struct IPC ipc;
    memset(&ipc,0,sizeof(struct IPC));
    read(readPipe,&ipc,sizeof(struct IPC));

    switch(ipc.type[0]){
        case '1':startNewConnection(&ipc);break;
    }
}

void startNewConnection(struct IPC *ipc){
    strcpy(serverIP,ipc->nl.nodes[0]);
}

int main(){
    int pipe1[2];//client.c read clientSegment function writes
    int pipe2[2];//clientSegment function read client.c writes

    pipe(pipe1);
    pipe(pipe2);
    readPipe = pipe1[0];
    writePipe = pipe2[0];
    int status;
    getIP();
    printf("My IP: %s",ipaddr);

    signal(SIGUSR1,signalHandler);
    while(1){
        if(fork()==0){//child process
            printf("Connecting with server of IP: %s\n",serverIP);
            clientSegment(serverIP,pipe1[1],pipe2[0]);
        }
        wait(&status);
    }
}