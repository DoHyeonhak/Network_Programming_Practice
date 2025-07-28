#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>		// opendir, readdir.etc.
#include <sys/stat.h>	// stat, mkdir
#include <errno.h>		// check error
#include <sys/time.h>
#include <unistd.h>

#define BUF_SIZE 30
#define LIST_SIZE 32

typedef struct{
    unsigned int seq;
    unsigned int data_len;
    unsigned char data[1024];
}pkt;

typedef struct{
    long f_size;
    char f_name[32];
}f_pkt;

f_pkt list[LIST_SIZE];  // file list

int list_cnt = 0;       // list cnt
int seq = 0;            // for seq check
int num = 0;            // chosen file's idx
int ack = 0;            // ack

void error_handling(char *message);
void DISPLAY_LIST();
void READ_DIR();
int SELECT_FILE();

int main(int argc, char *argv[])
{
    
    struct timeval  tv;
	double begin, end;

    int sock;           // socket
    
    socklen_t adr_sz;
    struct sockaddr_in serv_adr, from_adr;
    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // socket
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("UDP socket creation error");

    // set socket option - Timeout    // https://blog.naver.com/cjw8349/20167530321
    struct timeval optVal = {2, 0}; // 초, 밀리초
    int optLen = sizeof(optVal);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &optVal, optLen);

    // bind
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_adr.sin_port=htons(atoi(argv[2]));

    // read files in dir
    READ_DIR();

    // display list
    DISPLAY_LIST();

    // select file
    num = SELECT_FILE();
    
    printf("\n> %-18s  | %ld\n\n", list[num].f_name, list[num].f_size);

    // file open
    FILE* fp = NULL;
    fp = fopen(list[num].f_name, "rb");
    if(fp == NULL) {
        printf("file open error\n");
        exit(1);
    }
    
    
    int read_cnt = 0;
    int total_len = 0;

    gettimeofday(&tv, NULL);
	begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    while(total_len < list[num].f_size){
        if(seq == 0)  {   // seq 가 0인 데이터에 대해서는 항상 파일 이름과 사이즈를 보내도록 한다.
            while(1){   // for file info
                printf("seq=%d (about file info)\n", seq);
                sendto(sock, &list[num], sizeof(list[num]), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
                adr_sz = sizeof(from_adr);
                int ret = recvfrom(sock, &ack, sizeof(ack), 0, (struct sockaddr*)&from_adr, &adr_sz);
                if(ret == -1){
                    printf("Timeout, Again\n");
                    continue;
                }
                printf("ack=%d\n", ack);
                break;
            }
            seq++;
            continue;
        }
        // create packet
        pkt packet;
        packet.seq = seq;
        read_cnt = fread((void*)packet.data, 1, 1024, fp);  // 1024 bytes read
        packet.data_len = read_cnt;
        
        while(1){
            printf("seq=%d, data_len=%d\n",packet.seq, packet.data_len);    // check seq and data length
            sendto(sock, &packet, sizeof(packet), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
            adr_sz = sizeof(from_adr);
            int ret = recvfrom(sock, &ack, sizeof(ack), 0, (struct sockaddr*)&from_adr, &adr_sz);
            if(ret == -1){  // Fail case, No Ack
                printf("Timeout, Again\n");
                continue;
            }
            printf("ack=%d\n", ack);
            break;
        }
        total_len += packet.data_len;
        seq++;
    }
    gettimeofday(&tv, NULL);
	end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    
    // display result
    printf("\nlen: %d, filesize: %ld\n\n", total_len, list[num].f_size);
    
    double duration = (end - begin) / 1000;
    double throughput = ((double) total_len) / duration;
    
    printf("Time=%f\n", duration);
    printf("Throughput=%f\n\n", throughput);

    printf("Finish.\n");

    fclose(fp); // close file

    close(sock); // close socket
    return 0;
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
}

void DISPLAY_LIST(){
    printf("----- [LIST] -----\n");
    for(int i = 0; i < list_cnt; i++){
        printf("%d) %-18s | %ld\n", (i+1), list[i].f_name, list[i].f_size);
    }
}

void READ_DIR(){
    char* path = ".";
    char temp[16];

    // open dir
	DIR *dir = opendir(path);
	if(!dir){	// error check
		printf("opendir error\n");
		exit(1);
	}

    // process about each entry
	struct dirent *entry;
	while((entry=readdir(dir))!=NULL){
		// make full path
		char full_path[64] = "";
		strcpy(full_path, path);
		strcat(full_path, "/");
		strcat(full_path, entry->d_name);

		// Retrieve the status of the entry
		struct stat st;
		if(stat(full_path, &st)==-1){
			printf("stat error");
			exit(1);
        }
        // assign filename and size to list
        strcpy(list[list_cnt].f_name, entry->d_name);
        list[list_cnt++].f_size = st.st_size;   
	}
	// close dir
	closedir(dir);
}

int SELECT_FILE(){
    int num = 0;
    while(1){
        printf("\nInput file number : ");
        scanf("%d", &num);
        if(num == 0 || num > list_cnt){
            printf("\nDoes not exist. Again..\n");
        }
        break;
    }
    return (num-1);
}