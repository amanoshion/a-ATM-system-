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

#define S 24
#define M 64
#define L 256

char *names_opt[] = {"Register", "Login", "Delete_account", "Query_self", "Query_log", "Deposit", "Withdraw", "Transfer", "Quit"};

char *names_result[] = {"Success", "Fail", "Tips"};

typedef enum Opt_Type{
        Register,
        Login,
        Delete_account,
        Query_self,
        Query_log,
        Deposit,
        Withdraw,
        Transfer,
        Quit
} opt_type;

typedef struct MSG {
        opt_type opt_type;
        unsigned char dst[S];
        unsigned long data;
} MSG;

typedef enum Result_Type {
        Fail = -1,
        Success = 0,
        Unknown = 1
} Result_Type;

typedef struct Result_MSG {
        Result_Type result_type;
        char explain_msg[L];
} Result_MSG;

void show_intro() {
        printf("1) \tRegister\n");
        printf("2) \tLogin\n");
        printf("3) \tdelete account\n");
}

void show_operation() {
        printf("1) \tQuery\n");
        printf("2) \tDeposit\n");          
        printf("3) \tWithdraw\n");     
        printf("4) \tTransfer\n");
        printf("5) \tQuit\n");
}

void send_func(int clientfd, MSG *pmsg) {
        send(clientfd, pmsg, sizeof(*pmsg), 0);
        memset(pmsg, 0, sizeof(*pmsg));
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
                if (status == 1) {              // menu input
                        MSG msg;
                        msg.data = 0;
                        memset(msg.dst, 0, sizeof(msg.dst));

                        show_intro();
                        scanf("%d", &choose_menu);
                        char dst_buf[L] = {0};
                        switch(choose_menu) {
                                case 1:         // Register
                                        msg.opt_type = Register;

                                        send_func(clientfd, &msg);
                                        continue;
                                case 2:         // Login
                                        msg.opt_type = Login;

                                        printf("input your secret key\n");
                                        fgets(dst_buf, L, stdin);
                                        dst_buf[strcspn(dst_buf, "\n")] = '\0';
                                        strncpy(msg.dst, dst_buf, L);
                                        memset(dst_buf, 0, L);

                                        printf("input password\n");
                                        scanf("%lu", &msg.data);

                                        send_func(clientfd, &msg);
                                        continue;
                        }
                } else if (status == 2) {       // operation input
                        // ini msg
                        MSG msg;
                        msg.data = 0;
                        memset(msg.dst, 0, sizeof(msg.dst));

                        show_operation();
                        scanf("%d", &choose_operation);
                        char dst_buf[L] = {0};
                        switch(choose_operation) {
                                case 1:         // delete account
                                        msg.opt_type = Delete_account;

                                        printf("input your secret key\n");
                                        fgets(dst_buf, L, stdin);
                                        dst_buf[strcspn(dst_buf, "\n")] = '\0';
                                        strncpy(msg.dst, dst_buf, L);
                                        memset(dst_buf, 0, L);

                                        send_func(clientfd, &msg);
                                        continue;
                                case 2:         // query self
                                        msg.opt_type = Query_self;
                                        send_func(clientfd, &msg);
                                        continue;
                                case 3:         // query log
                                        msg.opt_type = Query_log;
                                        continue;
                                case 4:         // deposit
                                        msg.opt_type = Deposit;

                                        printf("input money for deposit\n");
                                        scanf("%lu", &msg.data);

                                        send_func(clientfd, &msg);
                                        continue;
                                case 5:         // withdraw
                                        msg.opt_type = Withdraw;
                                        printf("input money for withdraw\n");
                                        scanf("%lu", &msg.data);

                                        send_func(clientfd, &msg);
                                        continue;

                                case 6:                 // Transfer
                                        msg.opt_type = Transfer;

                                        printf("input dst account s public key\n");
                                        fgets(dst_buf, L, stdin);
                                        dst_buf[strcspn(dst_buf, "\n")] = '\0';
                                        strncpy(msg.dst, dst_buf, L);
                                        memset(dst_buf, 0, L);

                                        printf("input money account you want to transfer\n");
                                        scanf("%lu", &msg.data);

                                        send_func(clientfd, &msg);
                                        continue;
                                case 7:                 // quit
                                        msg.opt_type = Quit;
                                        break;
                        }
                }
                Result_MSG result_msg;
                int ret_recv = recv(clientfd, &result_msg, sizeof(result_msg), 0);
                if (ret_recv == -1) {
                        perror("recv fail");
                } else {
                        printf("recv msg success\n");
                }
                // print result
                printf("%s: %s", names_result[result_msg.result_type], result_msg.explain_msg);

        }
        return 0;
}