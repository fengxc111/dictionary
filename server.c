#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sqlite3.h>

#define N 32

/**
 * command types 
 **/
#define R 1     // register
#define L 2     // login
#define Q 3     // query
#define H 4     // history

#define DATABASE "user.db"

typedef struct {
    int type;
    char name[N];
    char data[256];
}MSG;

void signal_handler(int sig);
int func_client(int, sqlite3*);
int func_register(int, MSG*, sqlite3*);
int func_login(int, MSG*, sqlite3*);
int func_query(int, MSG*, sqlite3*);
int func_history(int, MSG*, sqlite3*);
int searchword(int, MSG*, char[]);
int get_date(char*);

int main(int argc, char** argv)
{
    int sockfd;
    struct sockaddr_in serveraddr;
    sqlite3 *db;
    int acceptfd;
    pid_t pid;

    if (argc != 3){
        printf("usage: %s <serverip> <ip>\n", argv[0]);
        return -1;
    }

    if (sqlite3_open("user.db", &db) != SQLITE_OK){
        printf("%s\n", sqlite3_errmsg(db));
        return -1;
    }else{
        printf("open DATABASE successfully\n");
    }

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("fail to create socket\n");
        return -1;
    }
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[2]));
    // serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
        perror("fali to bind socket\n");
        return -1;
    }

    if (listen(sockfd, 5) < 0){
        printf("fail to listen\n");
        return -1;
    }

    // signal(SIGCHLD, SIG_IGN);
    signal(SIGCHLD, signal_handler);

    while (1){
        if ( (acceptfd = accept(sockfd, NULL, NULL)) < 0){
            perror("fail to accept\n");
            return -1;
        }

        if ( (pid = fork()) < 0){
            printf("fail to fork()\n");
            return -1;
        }else if (pid == 0){    // child process
            close(sockfd);
            func_client(acceptfd, db);
        }else{      // father process
            close(acceptfd);
        }
    }
}

int func_client(int acceptfd, sqlite3* db){
    MSG msg;
    while (recv(acceptfd, &msg, sizeof(MSG), 0) > 0){
        switch(msg.type){
            case R:
                func_register(acceptfd, &msg, db);
                break;
            case L:
                func_login(acceptfd, &msg, db);
                break;
            case Q:
                func_query(acceptfd, &msg, db);
                break;
            case H:
                func_history(acceptfd, &msg, db);
                break;
            default:
                printf("Invalid msg\n");
        }
    }

    close(acceptfd);
    exit(0);

    return 0;
}

int func_register(int acceptfd, MSG* msg, sqlite3* db){
    char* errmsg;
    char sql[128];

    sprintf(sql, "insert into user values('%s', '%s')", msg->name, msg->data);
    printf("%s\n", sql);

    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
        printf("%s\n", errmsg);
        strcpy(msg->data, "user name already existed");
    }else{
        printf("client register success\n");
        strcpy(msg->data, "OK");
    }

    if (send(acceptfd, msg, sizeof(MSG), 0) < 0){
        perror("fail to send");
        return -1;
    }

    return 0;
}
int func_login(int acceptfd, MSG* msg, sqlite3* db){
    // printf("loading...\n");
    char* errmsg;
    char sql[128];
    int nrow, ncol;
    char **result;

    sprintf(sql, "select * from user where name = '%s' and passwd = '%s'", msg->name, msg->data);
    printf("%s\n", sql);

    if (sqlite3_get_table(db, sql, &result, &nrow, &ncol, &errmsg) != SQLITE_OK){
        printf("%s\n", errmsg);
        // return -1;
        // strcpy(msg->data, errmsg);
    }else{
        printf("get_table success\n");
    }

    if (nrow == 1){
        strcpy(msg->data, "OK");
        if (send(acceptfd, msg, sizeof(MSG), 0) < 0){
            printf("send() error\n");
            return -1;
        }
        return 1;
    }else{
        strcpy(msg->data, "name or password wrong.");
        if (send(acceptfd, msg, sizeof(MSG), 0) < 0){
            printf("send() error\n");
            return -1;
        }
    }

    return 0;
}
int searchword(int acceptfd, MSG* msg, char word[]){
    FILE *fp;
    int len = 0;
    char temp[512];
    int result;
    char *p;

    if ( (fp = fopen("dict.txt", "r")) == NULL){
        perror("fail to fopen()");
        strcpy(msg->data, "Failed to open dict.txt");
        send(acceptfd, msg, sizeof(MSG), 0);
        return -1;
    }

    len = strlen(word);
    // printf("%s, len = %d\n", word, len);

    while (fgets(temp, 512, fp) != NULL){
        // printf("temp: %s\n", temp);

        result = strncmp(temp, word, len);
        if (result < 0){
            continue;
        }
        if (result > 0 || ((result == 0) && (temp[len] !=' '))){
            break;
        }

        p = temp + len;
        // printf("find word %s\n", word);
        while (*p == ' '){
            p++;
        }
        strcpy(msg->data, p);
        printf("ready to send: %s\n", msg->data);

        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}
int get_date(char *date){
    time_t t;
    struct tm *tp;

    time(&t);
    tp = localtime(&t);
    sprintf(date, "%d-%d-%d %d:%d:%d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    printf("get date: %s\n", date);

    return 0;
}
int func_query(int acceptfd, MSG* msg, sqlite3* db){
    // printf("loading...\n");
    char word[64];
    int found = 0;
    char date[128];
    char sql[128];
    char *errmsg;

    strcpy(word, msg->data);

    found = searchword(acceptfd, msg, word);
    if (found == 1){
        get_date(date);
        sprintf(sql, "insert into record values('%s', '%s', '%s')", msg->name, date, word);
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
            printf("%s\n", errmsg);
            return -1;
        }else{
            printf("Insert record success\n");
        }
    }else{
        strcpy(msg->data, "Not found!");
    }

    if (send(acceptfd, msg, sizeof(MSG), 0) < 0){
        printf("send() error\n");
        return -1;
    }

    return 0;
}
int history_callback(void* arg, int f_num, char** f_value, char** f_name){
    // CREATE TABLE record(name text, date text, word text);

    int acceptfd;
    MSG msg;

    acceptfd = *((int *)arg);
    printf("%s, %s\n", f_value[1], f_value[2]);
    sprintf(msg.data, "%s, %s", f_value[1], f_value[2]);
    send(acceptfd, &msg, sizeof(MSG), 0);
    return 0;
}

int func_history(int acceptfd, MSG* msg, sqlite3* db){
    // printf("loading...\n");
    char sql[128];
    char *errmsg;

    sprintf(sql, "select * from record where name = '%s'", msg->name);
    printf("select * from record where name = '%s';\n", msg->name);
    if (sqlite3_exec(db, sql, history_callback, (void*)&acceptfd, &errmsg) != SQLITE_OK){
        printf("%s\n", errmsg);
    }else{
        printf("Query history success\n");
    }

    msg->data[0] = '\0';

    send(acceptfd, msg, sizeof(MSG), 0);

    return 0;
}
void signal_handler(int sig){
    printf("child process finish\n");
}