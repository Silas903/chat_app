// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 4434
#define MAX_CLIENTS 100
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define MAX_MESSAGE_LEN 1024

typedef struct {
    int sockfd;
    char username[MAX_USERNAME_LEN];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void broadcast(char *sender, char *message, int sender_sock) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sockfd != sender_sock) {
            char buffer[MAX_MESSAGE_LEN + MAX_USERNAME_LEN + 20];
            sprintf(buffer, "%s (group): %s", sender, message);
            send(clients[i].sockfd, buffer, strlen(buffer), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void send_private(char *sender, char *recipient, char *message, int sender_sock) {
    pthread_mutex_lock(&lock);
    int found = 0;
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].username, recipient) == 0) {
            char buffer[MAX_MESSAGE_LEN + MAX_USERNAME_LEN + 20];
            sprintf(buffer, "%s (private): %s", sender, message);
            send(clients[i].sockfd, buffer, strlen(buffer), 0);
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    if (!found) {
        char buf[100] = "Người nhận không tồn tại hoặc không online.\n";
        send(sender_sock, buf, strlen(buf), 0);
    }
}

int validate_account(const char *username, const char *password) {
    FILE *fp = fopen("accounts.txt", "r");
    if (!fp) return 0;
    char user[MAX_USERNAME_LEN], pass[MAX_PASSWORD_LEN];
    while (fscanf(fp, "%s %s", user, pass) != EOF) {
        if (strcmp(username, user) == 0 && strcmp(password, pass) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int register_account(const char *username, const char *password) {
    FILE *fp = fopen("accounts.txt", "r");
    if (fp) {
        char user[MAX_USERNAME_LEN], pass[MAX_PASSWORD_LEN];
        while (fscanf(fp, "%s %s", user, pass) != EOF) {
            if (strcmp(username, user) == 0) {
                fclose(fp);
                return 0; // User đã tồn tại
            }
        }
        fclose(fp);
    }
    fp = fopen("accounts.txt", "a");
    if (!fp) return 0;
    fprintf(fp, "%s %s\n", username, password);
    fclose(fp);
    return 1;
}

void *client_handler(void *arg) {
    int sock = *(int *)arg;
    char username[MAX_USERNAME_LEN];
    char buffer[MAX_MESSAGE_LEN + MAX_USERNAME_LEN + 100];

    // Đăng nhập hoặc đăng ký
    recv(sock, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "LOGIN", 5) == 0) {
        char *user = strtok(buffer + 6, " ");
        char *pass = strtok(NULL, " ");
        if (!validate_account(user, pass)) {
            send(sock, "Login failed\n", 13, 0);
            close(sock);
            return NULL;
        }
        strcpy(username, user);
        send(sock, "OK\n", 2, 0);
    } else if (strncmp(buffer, "REGISTER", 8) == 0) {
        char *user = strtok(buffer + 9, " ");
        char *pass = strtok(NULL, " ");
        if (!register_account(user, pass)) {
            send(sock, "Register failed (username exists)\n", 35, 0);
            close(sock);
            return NULL;
        }
        strcpy(username, user);
        send(sock, "Register successful. Please login again.\n", 41, 0);
        close(sock);
        return NULL;
    } else {
        send(sock, "Invalid command\n", 17, 0);
        close(sock);
        return NULL;
    }

    pthread_mutex_lock(&lock);
    clients[client_count].sockfd = sock;
    strcpy(clients[client_count].username, username);
    client_count++;
    pthread_mutex_unlock(&lock);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        if (strncmp(buffer, "GROUP ", 6) == 0) {
            broadcast(username, buffer + 6, sock);
        } else if (strncmp(buffer, "PRIVATE ", 8) == 0) {
            char *to = strtok(buffer + 8, " ");
            char *msg = strtok(NULL, "");
            if (to && msg)
                send_private(username, to, msg, sock);
        } else {
            char msg[] = "Sai cú pháp. Dùng GROUP <msg> hoặc PRIVATE <user> <msg>\n";
            send(sock, msg, strlen(msg), 0);
        }
    }

    // Ngắt kết nối
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sockfd == sock) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    close(sock);
    return NULL;
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("🚀 Server đang chạy ở cổng %d...\n", PORT);

    while (1) {
        client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, (void *)&client_sock);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
