#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30

typedef struct{
    unsigned int seq;
    unsigned int data_len;
    unsigned char data[1024];
}pkt;

typedef struct{
    long f_size;
    char f_name[32];
}f_pkt;


unsigned int ack = 0;

void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sock;

    socklen_t clnt_adr_sz;
    struct sockaddr_in serv_adr, clnt_adr;
    int str_len;
    
    f_pkt file;
    FILE *fp;


    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    
    clnt_adr_sz = sizeof(clnt_adr);

    str_len = recvfrom(serv_sock, &file, sizeof(file), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
    if(str_len == -1){
        error_handling("recvfrom error");
        exit(1);
    }
    sendto(serv_sock, &ack, sizeof(ack), 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz);
    
    printf("target=%s, size=%ld\n", file.f_name, file.f_size);
    printf("\ning...\n");
    int size = 0;
    char f_data[file.f_size];
    f_data[0] = '\0';

    // file open
    fp = fopen(file.f_name, "wb");
    if(fp == NULL) {
        printf("file open error\n");
        exit(1);
    }

    pkt prev_pkt;   // previous packet을 받아, ACK가 실패한 경우 처리하기
    // - recvfrom() : blocking
    //   - server to client data는 정상 보장
    //   - 정상적으로 받은 것이므로 쓴다. 하지만 같은 이전과 같은 seq일 때는?
    // - prev seq == current seq
    //   - client to server에서의 ACK 손실 의미
    //   - prev의 data는 이미 작성되었으므로, file에 데이터를 쓰지 말고 ACK만 다시 보낸다.
    while(size < file.f_size){
        // ack 손실 시 중복된 데이터가 연속해서 작성되는 문제를 해결하기
        pkt packet;
        str_len = recvfrom(serv_sock, &packet, sizeof(packet), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz); // blocking
        if(str_len == -1){
            error_handling("recvfrom error");
            exit(1);
        }
        ack = packet.seq;
        if(prev_pkt.seq == packet.seq){ // 이상한 경우
            sendto(serv_sock, &ack, sizeof(ack), 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz);
            continue;
        }
        size += packet.data_len;    // 중요
        fwrite(packet.data, 1, packet.data_len, fp);
        prev_pkt = packet;

        // normal case packet 전송
        sendto(serv_sock, &ack, sizeof(ack), 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz);
    }    
    
    printf("\nSuccess!\n");
        
    close(serv_sock);
    return 0;
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
}
