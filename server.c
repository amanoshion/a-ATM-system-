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

#include <sodium.h>     // generating Ed25519 key, need gcc -lsodium

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

void generate_account(unsigned long int passwd, Result_Type *result_type, char *explain_msg) {
        if (sodium_init() < 0) {
                fprintf(stderr, "libsodium init fail");
                return;
        }
        unsigned char seed[SEED_LEN];
        unsigned char secret_key[KEY_LEN];
        unsigned char public_key[KEY_LEN];

        randombytes_buf(seed, 32);

        crypto_box_seed_keypair(public_key, secret_key, seed);

        char pk_hex[KEY_LEN + 1];    
        char sk_hex[KEY_LEN + 1];    

        sodium_bin2hex(pk_hex, sizeof(pk_hex), public_key, KEY_LEN);
        sodium_bin2hex(sk_hex, sizeof(sk_hex), secret_key, KEY_LEN);
        //      write into db
        char sentence_insert_passwd[L] = {0};
        snprintf(sentence_insert_passwd, L, "INSERT INTO %s VALUES (DEFAULT, '%s', %lu);", PASSWD_TABLE_NAME, pk_hex, passwd);
        int ret_exec;
        ret_exec = sqlite3_exec(ppdb, sentence_insert_passwd, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                *result_type = Fail;
                return;
        }
        char sentence_insert_account[L] = {0};
        snprintf(sentence_insert_account, L, "INSERT INTO %s( '%s', '%s', 0);", ACCOUNT_TABLE_NAME, pk_hex, "Normal");        
        ret_exec = sqlite3_exec(ppdb, sentence_insert_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                *result_type = Fail;
                return;
        }

        *result_type = Success;
        strncpy(explain_msg, sk_hex, L);

        return;
}

char current_user_public_key[L] = {0};
void opt_login(MSG *msg, Result_Type *result_type, char *explain_msg, int *islogin) {
        unsigned char pk_new[KEY_LEN];
        unsigned long int passwd;
        
        crypto_box_keypair(pk_new, msg->dst);
        // check
        char sentence_search_public_key[L] = {0};
        snprintf(sentence_search_public_key, L, "SELECT %s FROM %s WHERE public_key = %s", "account_status", ACCOUNT_TABLE_NAME, pk_new);
        int ret = select_func(sentence_search_public_key);
        if (ret == ERROR || nrow != 1) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "no user or system error", L);
                *result_type = Fail;
                return;
        }
        if (!((strncmp(resultp[4], "Normal", 5) == 0) && (strlen(resultp[5]) == 5))) {

        }
        if (!(strncmp(resultp[3], pk_new, KEY_LEN) == 0)) {     
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal", L);        
                *result_type = Fail;
                return;
        } 
        
        *result_type = Success;
        *islogin = 1;
        return;
}

void opt_delete_account(char *public_key, Result_Type *result_type, char *explain_msg) {
        char sentence_delete_passwd[L] = {0};
        snprintf(sentence_delete_passwd, L, "DELETE FROM %s WHERE public_key = '%s';", PASSWD_TABLE_NAME, public_key);
        int ret_exec;
        ret_exec = sqlite3_exec(ppdb, sentence_delete_passwd, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                result_type = Fail;
                return;
        }
        char sentence_delete_account[L] = {0};
        snprintf(sentence_delete_account, L, "DELETE FROM %s WHERE public_key = '%s';", ACCOUNT_TABLE_NAME, public_key);        
        ret_exec = sqlite3_exec(ppdb, sentence_delete_account, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                result_type = Fail;
                return;
        }
        result_type = Success;
        return;
}

Result_Type handle_menu(int acceptfd, MSG *msg, char *explain_msg, int *islogin) {
        Result_Type result_type;
        switch(msg->opt_type) {
                case Register:  
                        generate_account(msg->data, &result_type, explain_msg);
                        break;
                case Login:
                        opt_login(msg, &result_type, explain_msg, islogin);
                        break;
                default:
                        break;
        }
        return;
}

Result_Type handle_client_opt(int acceptfd, MSG *msg, char *explain_msg) {   
        Result_Type result_type = Unknown;    
        
        check_illegal(msg, &result_type, explain_msg, current_user_public_key);         // check status and opt type

        opt_account_db(msg, &result_type, explain_msg, current_user_public_key);           
        opt_log_db(msg, &result_type);               

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
                        ini_db();               
                        int islogin = 0;
                        while(1) {
                                MSG msg;
                                int ret_recv = recv(acceptfd, &msg, sizeof(msg), 0);
                                if (ret_recv == -1) {
                                        perror("recv fail");
                                } else {
                                        printf("recv msg success\n");
                                }
                                if (!islogin) {
                                        Result_MSG result_msg;
                                        char explain_msg[L] = {0};
                                        // handle menu operation
                                        result_msg.result_type = handle_menu(acceptfd, &msg, explain_msg, &islogin);
                                        // check login
                                        if (result_msg.result_type == Success && islogin == 1) {
                                                strncpy(current_user_public_key, msg.dst,L);
                                        }

                                        strncpy(result_msg.explain_msg, explain_msg, L);
                                        // send to client
                                        send_func(acceptfd, &result_msg);
                                        if (msg.opt_type == Quit && result_msg.result_type == Success) {
                                                break;
                                        }
                                        // log if success
                                        if (result_msg.result_type == Success) {
                                                opt_log_db(&msg, msg.dst);
                                        }
                                } else if (islogin) {
                                        Result_MSG result_msg;
                                        // handle operation
                                        char explain_msg[L] = {0};
                                        result_msg.result_type = handle_client_opt(acceptfd, &msg, explain_msg);
                                        strncpy(result_msg.explain_msg, explain_msg, L);
                                        // send to client
                                        send_func(acceptfd, &result_msg);
                                        if (msg.opt_type == Quit && result_msg.result_type == Success) {
                                                islogin = 0;
                                        }
                                        // log if success
                                        if (result_msg.result_type == Success && msg.opt_type != Quit) {
                                                opt_log_db(&msg, msg.dst);
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
        sleep(120);      // dad wait for sons
        close(acceptfd);
        close(listenfd);
        return 0;
}
