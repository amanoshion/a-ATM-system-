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
// TODO key_word check: 5 error input or huge money operation

#define S 24
#define M 64
#define L 256

typedef enum Opt_Type{
        Query,
        Deposit,
        Withdraw,
        Transfer,
        Quit
} opt_type;
char *names_opt[5] = {"Query", "Deposit", "Withdraw", "Transfer", "Quit"};

typedef enum Result_Type {
        Success,
        Fail,
        Tips
}result_type;
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

void send_func(int acceptfd, opt_type type1, result_type type2, char *explain_msg) {
        buffer[M] = {0};
        strncat(buffer, names_opt[type1], S + 1);
        strncpy(buffer, names_result[type2], S + 1);
        if (explain_msg != NULL) {
                strncat(buffer, explain_msg, M + 1);
        }
        
        int send_ret = send(acceptfd, buffer, M, 0);
        if (send_ret == -1) {
                perror(send fail);
        }
        return;
}

void handle_client_opt(int acceptfd, opt_type type1, char *explain_msg, MSG *msg) {       
        result_type type2 = Tips;             // TODO : get type2 value
        send_func(acceptfd, type1, type2, "start");
        check_status();         // TODO : check if account is login
        opt_account_db(msg);           // TODO
        opt_log_db(msg);               // TODO
        send_func(acceptfd, type1, type2, "finish");
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
