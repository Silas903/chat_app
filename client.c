#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#define MAX 80
#define PORT 4434
#define SA struct sockaddr
void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n');
        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}

int main()
{
    int sockfd; //connfd;
    struct sockaddr_in servaddr;//cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))!= 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");

    char choice[16];
    char username[32], password[32];

    printf("Bạn muốn LOGIN hay REGISTER? ");
    scanf("%s", choice);
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
    char login_msg[128];
    snprintf(login_msg, sizeof(login_msg), "%s %s %s", choice, username, password);
    send(sockfd, login_msg, strlen(login_msg), 0);

    char response[64];
    recv(sockfd, response, sizeof(response), 0);
    if (strncmp(response, "Register successful. Please login again.", 50) == 0 || strncmp(response, "OK", 2) == 0) {
        printf("Đăng nhập thành công!\n");
    } else {
        printf("Đăng nhập/Đăng ký thất bại: %s\n", response);
        close(sockfd);
        return 1;
    }
    // function for chat
    func(sockfd);

    // close the socket
    close(sockfd);
}