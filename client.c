#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define N 32

/**
 * command types 
 **/
#define R 1     // register
#define L 2     // login
#define Q 3     // query
#define H 4     // history

typedef struct {
    int type;
    char name[N];
    char data[256];
}MSG;

int func_register(int sockfd, MSG *msg){
    msg->type = R;
    printf("Input your name:");
    scanf("%s", msg->name);
    getchar();
    printf("Input your password:");
    scanf("%s", msg->data);

    if (send(sockfd, msg, sizeof(MSG), 0) < 0){
        printf("fail to register\n");
        return -1;
    }
    if (recv(sockfd, msg, sizeof(MSG), 0) < 0){
        printf("fail to register\n");
        return -1;
    }

    printf("%s\n", msg->data);
    return 0;
}

int func_login(int sockfd, MSG* msg){
    msg->type = L;
    printf("Input your name:");
    scanf("%s", msg->name);
    getchar();
    printf("Input your password:");
    scanf("%s", msg->data);

    if (send(sockfd, msg, sizeof(MSG), 0) < 0){
        printf("fail to send\n");
        return -1;
    }
    if (recv(sockfd, msg, sizeof(MSG), 0) < 0){
        printf("fail to recv\n");
        return -1;
    }
    if (strncmp(msg->data, "OK", 2) == 0){
        printf("Login success\n");
        return 1;
    }else{
        printf("%s\n", msg->data);
    }

    return 0;
}

int func_query(int sockfd, MSG* msg){
    msg->type = Q;
    printf("--------------------\n");

    while (1){
        printf("Input word:");
        scanf("%s", msg->data);
        getchar();

        if (msg->data[0] == '#')
            break;
        
        if (send(sockfd, msg, sizeof(MSG), 0) < 0){
            printf("fail to send\n");
            return -1;
        }
        if (recv(sockfd, msg, sizeof(MSG), 0) < 0){
            printf("fail to receive\n");
            return -1;
        }
        printf("%s\n", msg->data);
    }
    return 0;
}

int func_history(int sockfd, MSG* msg){
    msg->type = H;
    send(sockfd, msg, sizeof(MSG), 0);
    int count = 0;
    while (1){

        recv(sockfd, msg, sizeof(MSG), 0);

        if (msg->data[0] == '\0')
            break;
        
        printf("%d: %s\n", ++count, msg->data);
        sleep(1);
    }
    return 0;
}

int main(int argc, char** argv)
{
    int sockfd;
    struct sockaddr_in serveraddr;
    int select;
    MSG msg;

    if (argc != 3){
        printf("usage: %s <serverip> <port>\n", argv[0]);
        return -1;
    }
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("fail to create a socket\n");
        return -1;
    }
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    // serveraddr.sin_port = htons(argv[2]);
    serveraddr.sin_port = htons(atoi(argv[2]));
    // serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    inet_pton(AF_INET, argv[1], &serveraddr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
        perror("fail to connect\n");
        return -1;
    }

    while(1){
        printf("--------------------\n");
        printf("** 1.register\n");
        printf("** 2.login\n");
        printf("** 3.quit\n");
        printf("--------------------\n");

        scanf("%d", &select);
        getchar();
        switch(select){
            case 1:
                func_register(sockfd, &msg);
                break;
            case 2:
                if (func_login(sockfd, &msg) == 1){
                    goto NEXT;
                }
                break;
            case 3:
                close(sockfd);
                system("cls");
                printf("\n\t\tbye!\t\t\n");
                exit(0);
                // break;
            default:
                printf("\ninvalid command\n");
        }
    }
NEXT:
    while(1){
        printf("--------------------\n");
        printf("** 1.query\n");
        printf("** 2.history\n");
        printf("** 3.quit\n");
        printf("--------------------\n");

        scanf("%d", &select);
        getchar();
        switch(select){
            case 1:
                func_query(sockfd, &msg);
                break;
            case 2:
                func_history(sockfd, &msg);
                break;
            case 3:
                close(sockfd);
                system("cls");
                printf("\n\t\tbye!\t\t\n");
                exit(0);
                // break;
            default:
                printf("\ninvalid command\n");
        }
    }
}