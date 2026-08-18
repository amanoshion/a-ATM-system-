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


typedef enum Opt_Type{
        Query,
        Deposit,
        Withdraw,
        Transfer,
        Quit
} opt_type;
typedef MSG {
        opt_type opt_type;
        char dst[S];
        unsigned long data;
} MSG;

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

        int choose_intro;
        int choose_operation;
        int status = 1;
        while(1) {
                // TODO
                if (status == 1) {
                        show_intro();
                        scanf("%d", &choose_intro);
                        switch(choose_intro) {
                                case 1:
                                        continue;
                                case 2:
                                        continue;
                                case 3:
                                        continue;
                        }
                } else if (status == 2) {
                        // ini msg
                        MSG msg;
                        memset(msg.data, 0, sizeof(msg.data));
                        memset(msg.dst, 0, sizeof(msg.dst));

                        show_operation();
                        scanf("%d", &choose_operation);
                        switch(choose) {
                                case 1:         
                                        continue;
                                case 2:
                                        continue;
                                case 3:
                                        continue;
                                case 4:                 // Transfer
                                        msg.opt_type = Transfer;

                                        char dst_buf[S] = {0};
                                        printf("input dst account public key\n");
                                        fgets(dst_buf, S, stdin);
                                        buf[strcspn(buf, "\n")] = '\0';
                                        strncpy(msg.dst, dst_buf, S);
                                        memset(dst_buf, 0, S);

                                        printf("input money account you want to transfer\n");
                                        scanf("%lu", &msg.data);

                                        send_func(clientfd, &msg);
                                        continue;
                                case 5:
                                        break;
                        }
                }

        }
        return 0;
}