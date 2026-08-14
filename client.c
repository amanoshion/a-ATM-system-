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

        while(1) {
                // TODO
                //
                //
        }
        return 0;
}