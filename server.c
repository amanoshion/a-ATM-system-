#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "db.h"

#define PORT 9999
#define SERVER_IP "192.168.1.100"

// static int error_count = 0;

typedef struct Result_MSG {
        Result_Type result_type;
        char explain_msg[L];
} Result_MSG;

int wstatus;
void sig_recycle(int signum) {
        waitpid(-1, &wstatus, 0);
}

void send_func(int acceptfd, Result_MSG *result_msg) {        
        int send_ret;
        
        send_ret = send(acceptfd, result_msg, sizeof(Result_MSG), 0);
        if (send_ret == -1) {
                perror("send fail");
        }
        return;
}


void opt_login(MSG *msg, Result_Type *result_type, char *explain_msg, int *islogin, char *current_public_key) { //current_public_key here for getting value
        unsigned char pk_bin[KEY_LEN] = {0};
        char pk_hex[KEY_HEX_LEN] = {0};
        unsigned char sk_bin[KEY_LEN] = {0};
        
        sodium_hex2bin(sk_bin, 32, msg->dst, 64, NULL, NULL, NULL);

        crypto_scalarmult_base(pk_bin, sk_bin);
        sodium_bin2hex(pk_hex, sizeof(pk_hex), pk_bin, KEY_LEN);
        // check passwd
        char sentence_check_passwd[L] = {0};
        snprintf(sentence_check_passwd, L, "SELECT passwd FROM %s WHERE public_key = '%s'", PASSWD_TABLE_NAME, pk_hex);

        int ret;
        ret = select_func(sentence_check_passwd);
        if (ret == ERROR || nrow != 1) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "no user or system error", L);
                *result_type = Fail;
                return;
        }

        unsigned long int passwd = char_to_int(resultp[1]);

        if (!(passwd == msg->data)) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal", L);        
                *result_type = Fail;
                return;
        }
        // check status
        char sentence_check_status[L] = {0};
        snprintf(sentence_check_status, L, "SELECT * FROM %s WHERE public_key = '%s'", ACCOUNT_TABLE_NAME, pk_hex);

        ret = select_func(sentence_check_status);
        if (ret == ERROR || nrow != 1) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "no user or system error", L);
                *result_type = Fail;
                return;
        }
        if (!((atoi(resultp[1]) == Normal) || atoi(resultp[1]) == Root)) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal", L);        
                *result_type = Fail;
                return;
        }
        strncpy(current_public_key, pk_hex, L);
        memset(explain_msg, 0, L);
        *result_type = Success;
        *islogin = 1;
        printf("a client login in\n");

        opt_lock(current_public_key);   // lock 

        sqlite3_free_table(resultp);
        return;
}

Result_Type handle_menu(int acceptfd, MSG *msg, char *explain_msg, int *islogin, char *current_public_key) {
        memset(explain_msg, 0, L);
        Result_Type result_type = Success;
        switch(msg->opt_type) {
                case Register:  
                        generate_account(msg->data, explain_msg);
                        break;
                case Login:
                        opt_login(msg, &result_type, explain_msg, islogin, current_public_key);
                        break;
                case Login_root:
                        opt_login(msg, &result_type, explain_msg, islogin, current_public_key);
                        break;

                default:
                        break;
        }
        return result_type;
}

Result_Type handle_client_opt(int acceptfd, MSG *msg, char *explain_msg, char *current_public_key) {   
        Result_Type result_type = Success;    
        
        check_illegal(msg, &result_type, explain_msg, current_public_key);         // check status and opt type

        opt_account_db(msg, &result_type, explain_msg, current_public_key);           
        opt_log_db(msg, current_public_key);               

        return result_type;
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
                        char current_user_public_key[L] = {0};
                        int islogin = 0;
                        ini_db();

                        while(1) {
                                MSG msg;
                                int ret_recv = recv(acceptfd, &msg, sizeof(msg), 0);
                                if (ret_recv == -1) {
                                        perror("recv fail");
                                        continue;
                                } else if (ret_recv == 0){
                                        printf("client close connection\n");;
                                        break;
                                } else {
                                        printf("recv msg success\n");
                                }
                                if (!islogin) {
                                        Result_MSG result_msg = {0};
                                        char explain_msg[L] = {0};
                                        // handle menu operation
                                        result_msg.result_type = handle_menu(acceptfd, &msg, explain_msg, &islogin, current_user_public_key);
                                        if (result_msg.result_type == Success) {
                                                strncpy(result_msg.explain_msg, explain_msg, L);
                                        }
                                       
                                        // send to client
                                        send_func(acceptfd, &result_msg);
                                        // log if success
                                        if (result_msg.result_type == Success) {
                                                opt_log_db(&msg, current_user_public_key);
                                        }
                                } else if (islogin) {
                                        Result_MSG result_msg;
                                        // handle operation
                                        char explain_msg[L] = {0};
                                        result_msg.result_type = handle_client_opt(acceptfd, &msg, explain_msg, current_user_public_key);
                                        strncpy(result_msg.explain_msg, explain_msg, L);
                                        // send to client
                                        send_func(acceptfd, &result_msg);
                                        if (msg.opt_type == Quit && result_msg.result_type == Success) {
                                                islogin = 0;
                                                opt_defrost(current_user_public_key);   // status into normal
                                        }
                                        // log if success
                                        if ((result_msg.result_type == Success) && msg.opt_type != Quit) {
                                                opt_log_db(&msg, current_user_public_key);
                                        }
                                }
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

        sleep(180);      // dad wait for sons
        close(acceptfd);
        close(listenfd);
        return 0;
}
