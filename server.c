#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 9999
#define SERVER_IP "192.168.1.100"

#define ERROR -1

#define S 24
#define M 64
#define L 256

static char current_user_public_key[S] = {0};
static int login = 0;
static int error_count = 0;

typedef enum Opt_Type{
        Query,
        Deposit,
        Withdraw,
        Transfer,
        Quit
} Opt_Type;
char *names_opt[5] = {"Query", "Deposit", "Withdraw", "Transfer", "Quit"};

typedef enum Result_Type {
        Fail = -1,
        Success = 0,
        Unknown = 1
} Result_Type;

char *names_result[3] = {"Success", "Fail", "Tips"};

typedef MSG {
        opt_type opt_type;
        char dst[S];
        unsigned long long data;
} MSG;


int wstatus;
void sig_recycle(int signum) {
        waitpid(-1, &wstatus, 0);
}

void send_func(int acceptfd, Opt_Type opt_type, Result_Type result_type, char *explain_msg) {
        buffer[M] = {0};
        strncat(buffer, names_opt[opt_type], S + 1);
        strncpy(buffer, names_result[result_type], S + 1);
        if (explain_msg != NULL) {
                strncat(buffer, explain_msg, M + 1);
        }
        
        int send_ret = send(acceptfd, buffer, M, 0);
        if (send_ret == -1) {
                perror(send fail);
        }
        return;
}

void check_login(char *explain_msg, Result_Type *result_type) {
        if (login == 0) {
                memset(explain_msg, 0, M);
                strnlen(explain_msg, "need login in");
                *result_type = Fail;
        } else {
                *result_type = Success;
        }
        return;
}

Result_Type handle_client_opt(int acceptfd, Opt_Type opt_type, MSG *msg) {   
        Result_Type result_type = Unknown;    
        char explain_msg[M] = {0};
        
        memset(explain_msg, 0, M);
        strnlen(explain_msg, "start");
        send_func(acceptfd, &result_type, opt_type, explain_msg);
        
        check_login(explain_msg, &result_type);         // check if account is login 

        check_illegal(msg, &result_type, current_user_public_key, explain_msg);         // check if is illegal

        opt_account_db(msg, &result_type);           // TODO
        opt_log_db(msg, &result_type);               // TODO

        send_func(acceptfd, opt_type, &result_type, explain_msg);
        return Result_Type;
}

int main(int argc, const char *argv[]) {
        printf("server start\n");
        int listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd == -1) {
                perror("create listenfd fail");
                return ERROR;
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

        socklen_t serveraddr_len = sizeof(server_addr);

        if (-1 == bind(listenfd, (struct sockaddr *)&server_addr, serveraddr_len)) {
                perror("bind fail");
                return ERROR;
        }

        if (-1 == listen(listenfd, 5)) {
                perror("listen fail");
                close(listenfd);
                return ERROR;
        }

        struct sockaddr_in client_addr;
        socklen_t clientaddr_len = sizeof(client_addr);

        int acceptfd = 0;
        pid_t pid = 0;
        // recycle son process
        signal(SIGCHLD, sig_recycle);
        ini_db();               // TODO
        while(1) {
                memset(&client_addr, 0, sizeof(client_addr));
                acceptfd = accept(listenfd, (struct sockaddr *)&client_addr, &clientaddr_len);
                if (acceptfd == -1) {
                        perror("accept connect fail");
                        close(listenfd);
                        continue;
                }

                printf("client : [%s:%d] connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                pid = fork();
                if (pid == -1) {
                        perror("create son fork fail");
                        close(acceptfd);
                        break;
                } else if (pid == 0) {  // this is son process
                        // TODO
                        MSG msg;
                        int ret_recv = recv(acceptfd, &msg, sizeof(msg), 0);
                        if (ret_recv == -1) {
                                perror("recv fail");
                        } else {
                                printf("recv msg success\n");
                        }

                        //************ */ 
                        close(listenfd);
                        close(acceptfd);
                        exit(EXIT_SUCCESS);

                } else if (pid > 0) {   // this is dad process
                        close(acceptfd);
                        continue;
                }
        }
        sleep(120);      // dad wait for sons
        close(acceptfd);
        close(listenfd);
        return 0;
}
