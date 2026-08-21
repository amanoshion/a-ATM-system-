#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define PORT 9999
#define IP_ADDRESS "192.168.1.100"

#define ERROR -1

#define S 128
#define M 256
#define L 1024

char *names_opt[] = {"Register", "Login", "Quit", "Delete_account", "Query_self", "Query_log", "Deposit", "Withdraw", "Transfer"};

char *names_result[] = {"Fail", "Success"};

typedef enum Opt_Type{
        Register,
        Login,
        Delete_account,
        Query_self,
        Query_log,
        Deposit,
        Withdraw,
        Transfer,
        Quit,
        Freeze,
        Defrost,
        Query_log_root,
        Login_root
} opt_type;

typedef struct MSG {
        opt_type opt_type;
        unsigned char dst[L];
        unsigned long int data;
} MSG;

typedef enum Result_Type {
        Fail = 0,
        Success = 1,
} Result_Type;

typedef struct Result_MSG {
        Result_Type result_type;
        char explain_msg[L];
} Result_MSG;

void show_menu() {
        printf("1) \tRegister\n");
        printf("2) \tLogin\n");
        printf("3) \tLogin in root\n");
}

void show_operation() {
        printf("1) \tDelete account\n");
        printf("2) \tQuery_self\n");
        printf("3) \tQuery_log\n");
        printf("4) \tDeposit\n");          
        printf("5) \tWithdraw\n");     
        printf("6) \tTransfer\n");
        printf("7) \tQuit\n");
}

void show_root() {
        printf("1) \tFreeze\n");
        printf("2) \tDefrost\n");
        printf("3) \tQuery_log_root\n");
        printf("4) \tQuit\n");
}

void send_func(int clientfd, MSG *pmsg) {
        send(clientfd, pmsg, sizeof(*pmsg), 0);
        // memset(pmsg, 0, sizeof(*pmsg));
}

int main(int argc, const char *argv[]) {
        printf("client start\n");
        int clientfd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == clientfd) {
                perror("create clientfd fail");
                return ERROR;
        }

        struct sockaddr_in sockaddr = {0};
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(PORT);
        sockaddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

        socklen_t clientsocklen = sizeof(sockaddr);
        int connect_ret = connect(clientfd, (const struct sockaddr*)&sockaddr, clientsocklen);
        if (connect_ret == -1) {
                perror("connect to server fail");
                return ERROR;
        }

        int choose_menu;
        int choose_operation;
        int status = 1;
        while(1) {
                MSG msg;
                msg.data = 0;
                memset(msg.dst, 0, sizeof(msg.dst));
                if (status == 1) {              // menu input
                        show_menu();
                        scanf("%d", &choose_menu);
                        // char dst_buf[L] = {0};
                        switch(choose_menu) {
                                case 1:         // Register
                                        printf("input password\n");
                                        scanf("%lu", &msg.data);
                                        msg.opt_type = Register;

                                        break;
                                case 2:         // Login
                                        msg.opt_type = Login;

                                        printf("input your secret key\n");
                                        // fgets(dst_buf, L, stdin);
                                        scanf("%s", msg.dst);
                                        // dst_buf[strcspn(dst_buf, "\n")] = '\0';
                                        // strncpy(msg.dst, dst_buf, L);
                                        // memset(dst_buf, 0, L);

                                        printf("input password\n");
                                        scanf("%lu", &msg.data);

                                        break;
                                case 3:         // login in root
                                        msg.opt_type = Login_root;

                                        printf("input your secret key\n");
                                        // fgets(dst_buf, L, stdin);
                                        scanf("%s", msg.dst);
                                        // dst_buf[strcspn(dst_buf, "\n")] = '\0';
                                        // strncpy(msg.dst, dst_buf, L);
                                        // memset(dst_buf, 0, L);

                                        printf("input password\n");
                                        scanf("%lu", &msg.data);

                                        break;
                        }
                } else if (status == 2) {       // operation input

                        show_operation();
                        scanf("%d", &choose_operation);
                        char dst_buf[L] = {0};
                        switch(choose_operation) {
                                case 1:         // delete account
                                        msg.opt_type = Delete_account;

                                        break;
                                case 2:         // query self
                                        msg.opt_type = Query_self;
                                        break;
                                case 3:         // query log
                                        msg.opt_type = Query_log_root;
                                        break;
                                case 4:         // deposit
                                        msg.opt_type = Deposit;

                                        printf("input money for deposit\n");
                                        scanf("%lu", &msg.data);

                                        break;
                                case 5:         // withdraw
                                        msg.opt_type = Withdraw;
                                        printf("input money for withdraw\n");
                                        scanf("%lu", &msg.data);

                                        break;

                                case 6:                 // Transfer
                                        msg.opt_type = Transfer;

                                        printf("input dst account s public key\n");
                                        scanf("%s", dst_buf);
                                        dst_buf[strcspn(dst_buf, "\n")] = '\0';
                                        strncpy(msg.dst, dst_buf, L);
                                        memset(dst_buf, 0, L);

                                        printf("input money account you want to transfer\n");
                                        scanf("%lu", &msg.data);

                                        break;
                                case 7:                 // quit
                                        status = 1;
                                        msg.opt_type = Quit;
                                        
                                        break;
                        }
                } else if (status == 3) {
                        show_root();
                        scanf("%d", &choose_menu);
                        char dst_buf[L] = {0};
                        switch(choose_menu) {
                                case 1:         // Register
                                        msg.opt_type = Freeze;
                                        printf("input public key\n");
                                        scanf("%s", msg.dst);
                                        break;
                                case 2:         // Login
                                        msg.opt_type = Defrost;
                                        printf("input public key\n");
                                        scanf("%s", msg.dst);
                                        break;

                                case 3:         // query full log
                                        msg.opt_type = Query_log_root;
                                        break;

                                case 4:                 // quit
                                        status = 1;
                                        msg.opt_type = Quit;
                                        break;
                        }
                }
                send_func(clientfd, &msg);


                Result_MSG result_msg;
                int ret_recv = recv(clientfd, &result_msg, sizeof(result_msg), 0);
                if (ret_recv == -1) {
                        perror("recv fail");
                } else {
                        printf("recv msg success\n");
                }
                if ((result_msg.result_type == Success) && (msg.opt_type == Login)) {
                        status = 2;
                } else if ((result_msg.result_type == Success) && (msg.opt_type == Login_root)) {
                        status = 3;
                }
                // print result
                printf("%s: %s\n", names_result[result_msg.result_type], result_msg.explain_msg);

        }
        return 0;
}