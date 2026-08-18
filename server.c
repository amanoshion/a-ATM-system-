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

#include <sodium.h>     // generating Ed25519 key need gcc -lsodium

#define PORT 9999
#define SERVER_IP "192.168.1.100"

#define ERROR -1

#define S 24
#define M 64
#define L 256

#define SEED_LEN 32
#define PK_LEN 32
#define SK_LEN 64
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
        char dst[M];
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

void generate_account(char *passwd, Result_MSG *result_msg, char *explain_msg) {
        if (sodium_init() < 0) {
                fprintf(stderr, "libsodium init fail")；
                return ERROR;
        }
        unsigned char seed[SEED_LEN];
        unsigned char secret_key[SK_LEN];
        unsigned char public_key[PK_LEN];

        randombytes_buf(seed, SEED_LEN);

        if (crypto_sign_seed_keypair(public_key, secret_key, seed) != 0) {
                fprintf(stderr, "generate key fail\n");
                return ERROR;
        }

        char pk_hex[PK_LEN * 2 + 1];    // 65 bytes
        char sk_hex[SK_LEN * 2 + 1];    // 129 bytes

        sodium_bin2hex(pk_hex, sizeof(pk_hex), public_key, PK_LEN);
        sodium_bin2hex(sk_hex, sizeof(sk_hex), secret_key, SK_LEN);
        //      TODO : write into db

        snprintf(sentence_insert_log, "INSERT INTO %s 
                VALUES (DEFAULT, %s, %s);", PASSWD_TABLE_NAME, pk_hex, passwd);
                

        int ret_exec = sqlite3_exec(ppdb, sentence_insert_log, NULL, NULL, &errmsg);
        if (ret_exec == SQLITE_ERROR) {
                printf("sqlite3_exec fail : %s", errmsg);
                result_msg->result_type = Fail;
                return;
        }
        // send
        strncpy(explain_msg, sk_hex);
        send_func(acceptfd, &result_msg);
        //*****make sure safe******** */
        memset(explain_msg, 0, L);

        sodium_memzero(seed, sizeof(seed));
        sodium_memzero(secret_key, sizeof(secret_key));
        sodium_memzero(sk_hex, sizeof(sk_hex));

        return OK;
}

static char current_user_public_key[S] = {0};
void opt_login(Result_Type *result_type, char *explain_msg) {
        
}
void opt_delete_account(Result_Type, *result_type, char *explain_msg) {

}

Result_Type handle_menu(int acceptfd, MSG *msg, char *explain_msg) {
        switch(msg->opt_type) {
                case Register:  // dont need send 
                        generate_account(msg->dst, explain_msg);
                        break;
                case Login:
                        break;
                case Delete_account:
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
                                Result_MSG result_msg;
                                if (!islogin) {
                                        // handle login operation
                                } else if (islogin) {
                                        // handle operation
                                        char explain_msg[L] = {0};
                                        result_msg.result_type = handle_client_opt(acceptfd, msg, &result_msg, explain_msg);
                                        strncpy(result_msg->explain_msg, explain_msg, M);
                                        // send to client
                                        send_func(acceptfd, &result_msg);
                                        if (msg->opt->type == Quit && result_msg->result_type == Success) {
                                                break;
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
