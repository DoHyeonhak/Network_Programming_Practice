#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h>
#include <pthread.h>

#include "Console.h"

#define BUF_SIZE 1024

char words[BUF_SIZE];
int idx = 0;        // 키보드 입력 시 정상 출력을 위한 인덱스 변수

int bool_esc = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* send_word(void* arg);
void* receive_word(void* arg);
void error_handling(char *message);

int main(int argc, char* argv[])
{
    
    words[0] = '\0';

    int sock;
    struct sockaddr_in serv_addr;
    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error!");



    // create thread (sender and receiver) - 별도로 하지 않으면 구현이 어려움
    pthread_t send_tid;
    pthread_t recv_tid;
    pthread_create(&send_tid, NULL, send_word, &sock);
    pthread_create(&recv_tid, NULL, receive_word, &sock);

    // display console 
    clrscr();
    while(1){

        if(bool_esc == 1){
            clrscr();
            printf("Bye!\n");
            return 1;
        }

        pthread_mutex_lock(&mutex);     // lock

        gotoxy(1,1);
        printf("\033[38;2;13;188;121m");
        printf("Search Word: ");
        printf("\033[0m");

        gotoxy(1,2);
        printf("------------------------------");        

        gotoxy(14, 1);
        printf("%s", words);
        fflush(stdout);        

        pthread_mutex_unlock(&mutex);   // unlock

        usleep(5000);   // ms
    }
    
    pthread_join(send_tid, NULL);
    pthread_join(recv_tid, NULL);

    clrscr();
    fflush(stdout);

    close(sock);
    return 0;
}

void* send_word(void* arg){
    
    int sock = *((int*)arg);
    char ch = 0;

    while(1){

        clrscr();
        
        ch = getch();   // read char

        if(ch == 27){
            bool_esc = 1;
            return NULL;
        }

        pthread_mutex_lock(&mutex);     // lock
        gotoxy(13 + idx, 1);
        printf("%c", ch); 
        pthread_mutex_unlock(&mutex);   // unlock

        if(ch == 8 || ch == 127){ // backspace
            if(idx > 0){    // 문자를 지웠을 때 idx도 감소
                idx--;
            }
        }else{
            words[idx++] = ch;
        }
        words[idx] = '\0';

        write(sock, words, idx);  
    }

    return NULL;
}

void* receive_word(void* arg){
    
    int sock = *((int*)arg);

    int word_len = 0;
    int cnt = 0;
    int ret = 0;

    char prev_word[BUF_SIZE];   // 특정 문자열에 색깔을 입히기 위한 비교 대상
    prev_word[0] = '\0';

    while(1){

        read(sock, &cnt, sizeof(int)); 
        
        read(sock, &word_len, sizeof(int));

        read(sock, prev_word, word_len);
        prev_word[word_len] = '\0';


        char str[BUF_SIZE];
        str[0] = '\0';
        for(int i = 0; i < cnt; i ++){
            ret = read(sock, str, 64);
            if(ret == -1){
                printf("read() error");
                exit(1);
            }
            str[ret] = '\0';


            pthread_mutex_lock(&mutex); // lock
            gotoxy(1,3 + i);

            char* start = strstr(str, prev_word);   // 포함된 문자열 시작 위치
            for(int j = 0; j <= strlen(str); j++){
                if(&str[j] == start){
                    for(int k = j; k < j + word_len; k++){
                        printf("\033[38;2;245;245;67m");
                        printf("%c", str[k]);
                        printf("\033[0m");
                    }
                    j += word_len - 1;
                }else{
                    // printf("\033[38;2;255;255;255m");
                    printf("%c", str[j]);
                    // printf("\033[0m");
                }
            }
            printf("\n");
            pthread_mutex_unlock(&mutex); // unlock

        }
    }

    return NULL;
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
}

    