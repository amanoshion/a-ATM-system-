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

#define ERROR -1

#define S 24
#define M 64
#define L 256

#define SEED_LEN 33
#define KEY_LEN 33
// static int error_count = 0;

typedef enum Opt_Type{
        Register,
        Login,
        Delete_account,
        Query,
        Deposit,
        Withdraw,
        Transfer,
        Quit
} Opt_Type;
char names_opt[] = {"Register", "Login", "Delete_account", "Query", "Deposit", "Withdraw", "Transfer", "Quit"};

typedef struct Result_Type {
        Fail,
        Success,
        Unknown
} Result_Type;

char *names_result[3] = {"Success", "Fail", "Tips"};

typedef MSG {
        opt_type opt_type;
        char dst[L];
        unsigned long long data;
} MSG;

typedef Result_MSG {
        Result_Type result_type,
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

void generate_account(char *passwd, Result_Type *result_type, char *explain_msg) {
        if (sodium_init() < 0) {
                fprintf(stderr, "libsodium init fail");
                return ERROR;
        }
        unsigned char seed[SEED_LEN];
        unsigned char secret_key[KEY_LEN];
        unsigned char public_key[KEY_LEN];

        randombytes_buf(seed, 32);

        crpto_box_seed_keypair(public_key, secret_key, seed);

        char pk_hex[KEY_LEN + 1];    
        char sk_hex[KEY_LEN + 1];    

        sodium_bin2hex(pk_hex, sizeof(pk_hex), public_key, PK_LEN);
        sodium_bin2hex(sk_hex, sizeof(sk_hex), secret_key, SK_LEN);
        //      write into db
        char sentence_insert_passwd[L] = {0};
        snprintf(sentence_insert_passwd, "INSERT INTO %s 
                VALUES (DEFAULT, %s, %s);", PASSWD_TABLE_NAME, pk_hex, passwd);
        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                *result_type = Fail;
                return;
        }
        char sentence_insert_account[L] = {0};
        snprintf(sentence_insert_account, "INSERT INTO %s
                ( %s, %s, %lu);", ACCOUNT_TABLE_NAME, pk_hex, "Normal", 0);        
        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                *result_type = Fail;
                return;
        }

        *result_type = Success;
        strncpy(explain_msg, sk_hex);

        return;
}

static char current_user_public_key[S] = {0};
void opt_login(MSG *msg, Result_Type *result_type, char *explain_msg, int *islogin) {
        unsigned char pk_new[PK_LEN];
        unsigned char pk_db[PK_LEN];
        unsigned long int passwd;
        
        crpto_box_keypair(pk_new, msg->dst);
        // check
        char sentence_search_public_key[M] = {0};
        snprintf(sentence_check_status, "SELECT %s FROM %s WHERE public_key = %s", "account_status", public_key, PASSWD_TABLE_NAME, pk_new);
        int ret = select_func(sentence_search_public_key);
        if (ret == ERROR || nrow != 1) {
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "no user or system error");
                *result_type == Fail;
                return;
        }
        if (!(((strncmp(resultp[4]), "Normal", 5) == 0) && (strlen(resultp[5]) == 5))) {

        }
        if (!((strncmp(resultp[3]), pk_new, KEY_LEN_LEN) == 0)) {     
                memset(explain_msg, 0, M);
                strncpy(explain_msg, "status abnormal");        
                *result_type == Fail;
                return
        } 
        
        *result_type == Succes;
        *islogin = 1;
        return;
}

void opt_delete_account(char *public_key, Result_Type *result_type, char *explain_msg) {
        char sentence_delete_passwd[L] = {0};
        snprintf(sentence_insert_passwd, "DELETE FROM %s 
                WHERE public_key = %s;", PASSWD_TABLE_NAME, msg->dst);
        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                result_msg->result_type = Fail;
                return;
        }
        char sentence_delete_account[L] = {0};
        snprintf(sentence_insert_account, "DELETE FROM %s
                WHERE public_key = %s;", ACCOUNT_TABLE_NAME, msg->dst);        
        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                result_msg->result_type = Fail;
                return;
        }
        result_msg->result_type = Success;
        return;
}

Result_Type handle_menu(int acceptfd, MSG *msg, Result_Type *result_type, char *explain_msg, int *islogin) {
        switch(msg->opt_type) {
                case Register:  // dont need send 
                        generate_account(msg->dst, result_type, explain_msg);
                        break;
                case Login:
                        opt_login(msg, result_type, explain_msg, islogin);
                        break;
                case Delete_account:
                        opt_delete_account(msg->dst, result_type, explain_msg);
                        break;
        }
}


Result_Type handle_client_opt(int acceptfd, MSG *msg, char *explain_msg) {   
        Result_Type result_type = Unknown;    
        
        check_illegal(msg, &result_type, explain_msg, current_user_public_key);         // check status and opt type

        opt_account_db(msg, &result_type, explain_msg);           
        opt_log_db(msg, &result_type);               

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
                        int islogin = 0
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

                                        // handle login operation
                                        result_msg.result_type = handle_menu(accptfd, &msg, explain_msg);
                                        strncpy(result_msg->explain_msg, explain_msg, M);
                                        // send to client
                                        send_func(acceptfd, &result_msg);
                                        if (msg->opt->type == Quit && result_msg->result_type == Success) {
                                                break;
                                        }
                                        // log if success
                                        if (result_msg.result == Success) {
                                                opt_log_db(msg, public_key);
                                        }
                                } else if (islogin) {
                                        Result_MSG result_msg;
                                        // handle operation
                                        char explain_msg[L] = {0};
                                        result_msg.result_type = handle_client_opt(acceptfd, msg, &result_msg, explain_msg);
                                        strncpy(result_msg->explain_msg, explain_msg, M);
                                        // send to client
                                        send_func(acceptfd, &result_msg);
                                        if (msg->opt->type == Quit && result_msg->result_type == Success) {
                                                *islogin = 0;
                                        }
                                        // log if success
                                        if (result_msg.result == Success && msg->opt_type != Quit) {
                                                opt_log_db(msg, public_key);
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
