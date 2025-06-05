#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h> //struct sockaddr_in cấu trúc lưu thông tin về địa chỉ mạng
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // socket()
#include <sys/types.h>
#include <unistd.h> //read(), write(), close()
#include <pthread.h>
#include <arpa/inet.h>

#define MAX 80
#define PORT 4434
#define MAX_CLIENTS 100
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define SA struct sockaddr

typedef struct {
    int sockfd;
    char username[MAX_USERNAME_LEN];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void func(int connfd);
void broadcast(char *sender, char *message, int sender_sock);
void send_private(char *sender, char *recipient, char *message, int sender_sock);
void *handle_client(void *args);
int check_credentials(const char *username, const char *password);

int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;
    pthread_t tid;

    //socker create and verification
    sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket creation failed ...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    // if(setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
    //     perror("setsockopt failed !!\n");
    //     exit(EXIT_FAILURE);
    // }
    bzero(&servaddr,sizeof(servaddr));

    //assign IP,PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    //Binding newly cr
    if((bind(sockfd, (SA*)&servaddr,sizeof(servaddr))) != 0){
        printf("Socket bind failed ...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Server is ready to listen and verification
    if((listen(sockfd,5)) != 0){
        printf("Listen failed !!\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    // len = sizeof(cli);
    //   connfd = accept(sockfd, (SA*)&cli, &len); 
    // if (connfd < 0) { 
    //     printf("server accept failed...\n"); 
    //     exit(0); 
    // } 
    // else
    //     printf("server accept the client...\n"); 
  
    // Function for chatting between client and server 
    //func(connfd);
    len = sizeof(cli);
    while(1)
    {
        connfd = accept(sockfd,(SA*)&cli,&len);
        if(connfd < 0){
            printf("Server accept failed ...\n");
            continue;
        }
        else
            printf("Server accept the client %s:%d\n",inet_ntoa(cli.sin_addr),ntohs(cli.sin_port));
        if(pthread_create(&tid,NULL,handle_client,&connfd) != 0){
            printf("pthread_create failed!! \n");
            close(connfd);
        }
        pthread_detach(tid); // Giải phóng tài nguyên tự động khi 1 thread kết thúc
    }
  
    // After chatting close the socket 
    close(sockfd);
    return 0;
}

//Chức năng được thiết kế cho chat giữa client và server
void func(int connfd)
{
    char buff[MAX]; 
    int n;
    char buffer[1024];
    char username[64],password[64];
    char action[16];
    //Nhập thông tin đăng nhập
    recv(connfd,buffer,sizeof(buffer),0);
    sscanf(buffer,"%s %s %s",action, username, password);

    if(strcmp(action,"REGISTER") == 0){
        FILE *file = fopen("account.txt","a+");
        char line[128],file_user[64];
        int exists = 0;
        
        while(fgets(line, sizeof(line), file)){
            sscanf(line,"%[^:]", file_user);
            if(strcmp(file_user,username) == 0){
                exists = 1;
                break;
            }
        }
        if(exists) {
            send(connfd, "EXISTS",6,0);
            fclose(file);
            close(connfd);
            return NULL;
        } else {
            fprintf(file,"%s:%s\n",username,password);
            fclose(file);
            send(connfd,"REGISTERED",10,0);
            }
    }
        else if(strcmp(action,"LOGIN") == 0){
            if (!check_credentials(username, password)) {
            send(connfd, "FAIL", 4, 0);
            close(connfd);
            return NULL;
            } else {
                send(connfd, "OK", 2, 0);
            }
        } else {
            send(connfd, "INVALID", 7, 0);
            close(connfd);
            return NULL;
        }
    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MAX); 
  
        // read the message from client and copy it in buffer 
        read(connfd, buff, sizeof(buff)); 
        // print buffer which contains the client contents 
        printf("From client: %s\t To client : ", buff); 
        bzero(buff, MAX); 
        n = 0; 
        // copy server message in the buffer 
        while ((buff[n++] = getchar()) != '\n') ; 
  
        // and send that buffer to client 
        write(connfd, buff, sizeof(buff)); 
  
        // if msg contains "Exit" then server exit and chat ended. 
        if (strncmp("exit", buff, 4) == 0) { 
            printf("Server Exit...\n"); 
            break; 
        } 
    }
}
void *handle_client(void *args)
{
    int connfd = *(int*)args;
    func(connfd);
    pthread_exit(NULL);
}

int check_credentials(const char *username, const char *password){
    FILE *file = fopen("account.txt","r");
    if(!file) return 0;

    char line[128];
    char file_user[MAX_USERNAME_LEN], file_pass[MAX_PASSWORD_LEN];
    while(fgets(line,sizeof(line),file)){
        sscanf(line,"%[^:]:%s",file_user,file_pass);
        if(strcmp(file_user,username)==0 && strcmp(file_pass,password)==0){
            fclose(file);
            return 1; // đúng
        }
    }
    fclose(file);
    return 0; //sai
}

