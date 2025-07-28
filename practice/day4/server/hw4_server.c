/*
	https://chanhuiseok.github.io/posts/algo-14/	
    
    문자열 검색을 위해서는 주로 trie structure or kmp를 사용
    그러나 문자열 검색 및 포함 상태(문자열 중간에도 가능해야함)에 대하여 suffix trie로 구현하기 복잡했음
    strstr()은 bruteforce 방식을 사용하므로, 이보다 조금 더 나은 kmp 알고리즘을 사용해보기
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include "kmp.h"

#define SET_SIZE 100
#define BUF_SIZE 256
#define MAX_CLNT 256

typedef struct{
    char str[64];
    int num;
}data_t;

// global variable
data_t dataset[SET_SIZE];
int set_cnt = 0;

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 

void* handle_clnt(void* arg);
void error_handling(char *message);
void read_save_data(FILE* fp);

int main(int argc, char *argv[])
{

    int serv_sock;
    int clnt_sock;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    FILE* fp = NULL;
    int i = 0;

    if (argc != 2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    read_save_data(fp); // handle file data

    // socket
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    
    
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");


    while(1){
        pthread_t tid;

        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        
        if (clnt_sock == -1)
            error_handling("accept() error");

        pthread_mutex_lock(&mutex);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutex);

        pthread_create(&tid, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(tid);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    }

    close(serv_sock);
    return 0;

}

void* handle_clnt(void* arg){

    int clnt_sock = *((int*)arg);

    char message[BUF_SIZE] = "";    // for sending lists to client 
    char str[256] = "";             // for receiving data
    int read_len = 0;

    while((read_len = read(clnt_sock, str, sizeof(str))) != 0){
        
        str[read_len] = '\0';
        message[0] = '\0';

        if(str[0] == ' '){  // 공백만 입력한 경우 거르기
            continue;
        }

        // 특정 문자열이 포함된 경우 데이터 저장 및 카운트
        int cnt_found = 0;        

        pthread_mutex_lock(&mutex); // lock
        
        data_t* data_found = (data_t*)malloc(sizeof(data_t) * set_cnt);

        for(int i = 0; i < set_cnt; i++){
            if(kmp(dataset[i].str, str)){   // 특정 문자열이 포함된 경우                
                data_found[cnt_found].str[0] = '\0';
                strcpy(data_found[cnt_found].str, dataset[i].str);
                data_found[cnt_found].num = dataset[i].num;
                cnt_found++;
            }
        }

        pthread_mutex_unlock(&mutex); // unlock

        // selection sort (1. 정수 내림차순, 2. 정수값이 같을 시 알파벳 순)
        int max = 0;
        for(int i = 0; i < cnt_found - 1; i++){
            max = i;
            for(int j = i + 1; j < cnt_found; j++){
                if(data_found[j].num > data_found[max].num){ // 내림차순
                    max = j;
                }else if(data_found[j].num == data_found[max].num){ // 정수값이 같을 때
                    if(strcmp(data_found[j].str, data_found[max].str) < 0){ // 알파벳 순서로 정렬
                        max = j;
                    }
                }
            }
            if(i != max){ // 변경 소요가 있을 때
                data_t temp = data_found[i];
                data_found[i] = data_found[max];
                data_found[max] = temp;
            }
        }

        int list_cnt = 0;  // 실제로 보낼 리스트 목록 갯수
        if(cnt_found > 10){ // 최대 10개 한정하기
            list_cnt = 10;
        }else{
            list_cnt = cnt_found;
        }

        // write data to client
        // 1. list_cnt, 2. str(클라이언트쪽에서 이게 업데이트 됐으면 색깔처리가 어려워짐. 클라이언트에서 뮤텍스 걸어서 한다고 해도 보장 X) 3. 문자열

        write(clnt_sock, &list_cnt, sizeof(int));        
        write(clnt_sock, &read_len, sizeof(int));
        write(clnt_sock, str, strlen(str));
        
        // printf("list_cnt=%d\n", list_cnt);
        // printf("len=%d\n", read_len);
        // printf("str=%s\n", str);

        for(int i = 0; i < list_cnt; i++){
            write(clnt_sock, data_found[i].str, 64);
            // printf("msg=%s\n", data_found[i].str);
        }

        // data_found deallocate
        free(data_found);
    }

    pthread_mutex_lock(&mutex);
    for(int i = 0; i < clnt_cnt; i++){  // remove disconnected client
        if(clnt_sock == clnt_socks[i]){
            while(i++ < clnt_cnt-1){
                clnt_socks[i] = clnt_socks[i+1];
            }
            break;  
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutex);

    close(clnt_sock);

    return NULL;
}

void read_save_data(FILE* fp){  // 공유 변수를 사용하지만 소캣 연결 전에 진행
    
    char line[256] = "\0";

    fp = fopen("data.txt", "rb");
    if(fp == NULL){
        printf("file open error\n");
        exit(1);
    }

    while(fgets(line, sizeof(line), fp) != NULL){
        char* ptr = strtok(line, "|");
        char* cnt_ptr = strtok(NULL, "\0");

        strcpy(dataset[set_cnt].str, ptr);
        dataset[set_cnt].num = atoi(cnt_ptr);
        printf("%s %d\n", dataset[set_cnt].str, dataset[set_cnt].num);
        
        set_cnt++;
    }
    printf("set_cnt=%d\n\n", set_cnt);

    fclose(fp);
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
}
