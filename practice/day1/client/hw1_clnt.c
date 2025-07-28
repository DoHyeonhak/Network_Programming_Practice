#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUF_SIZE 30
#define LIST_SIZE 32

typedef struct{
    char f_name[32];
    long f_size;
}FILE_IN_DIR;

FILE_IN_DIR files[LIST_SIZE];  // list variable
int cnt = 0;    // list counting variable


void error_handling(char *message);


int main(int argc, char *argv[])
{
    char msg[2048] = "";  // read for file list   

    int sd;
    FILE *fp;

    // char size[BUF_SIZE]; // for reading file size
    char str[30] = "";  // input file name that we want
    
    long filesize = 0;  // file size
    int read_len = 0;

    struct sockaddr_in serv_adr;

    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // socket
    sd = socket(PF_INET, SOCK_STREAM, 0);
    
    // bind
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    // connect
    connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    
    // program loop
    while(1){
        printf("[File Transfer Program]\n");
        printf("press a key between 1 and 9 to start and continue ( 0 is for Quit )\n");
        int num = 0;
        printf("> ");
        scanf("%d", &num);
        getchar();
        if(!num){   // press 0
            char progress[] = "exit";
            write(sd, &progress, sizeof(progress));
            printf("Bye!\n");
            break;
        }else{
            char progress[] = "1";
            write(sd, &progress, sizeof(progress));
        }

        // Read file list and assign data to struct type variable
        msg[0] = '\0';  // initialize for loop
        read_len = read(sd, &msg, 2048);
        if (read_len == -1){
            error_handling("read() error!");
            exit(1);
        }
        
        printf("[LIST]\n");
        cnt = 0;    // initialize for loop
        char* ptr = strtok(msg, "\n");  // split message to '\n'
        while(ptr!=NULL){   // loop for splitting
            sscanf(ptr, "%s %ld", files[cnt].f_name, &files[cnt].f_size);   // split data (name, size)
            ptr = strtok(NULL, "\n"); // split message to '\n'
            printf("%-16s | %ld\n", files[cnt].f_name, files[cnt].f_size);   // print file info (name, size)
            cnt++;  // increase list index
        }
        putchar('\n');

        while(1){   // loop for reading file name and checking if this exist
            printf("Input file name\n> ");
            fgets(str, sizeof(str), stdin);
            str[strlen(str) - 1] = '\0';
            int pass = 0;   // boolean role

            for(int i = 0; i < cnt; i++){   // checking if this exit
                if(strcmp(str, files[i].f_name) == 0){  // 찾은 경우
                    filesize = files[i].f_size; // assign f_size to filesize
                    printf("\tfile = %s\n", str); // print file name
                    printf("\tfilesize = %ld\n", filesize); 
                    pass = 1;
                    break;
                }
            }
            
            if(!pass){  // fail
                printf("This does not exist..\n");
                continue;
            }else{  // success
                break;
            }
        }

        write(sd, &str, 30);    // 파일 이름 보내기

        printf("\nWait...\n\n");

        // file open
        fp = fopen(str, "wb");
        if(fp == NULL) {
            printf("file open error\n");
            exit(1);
        }
    
        // read file data
        char buf[256] = "";
        int nbyte = 0;
        while(nbyte < filesize){
            read_len = read(sd, &buf, 256);    // 파일사이즈만큼
            if (read_len == -1){
                error_handling("read() error!");
                exit(1);
            }
            nbyte += read_len;
            fwrite(buf, 1, read_len, fp);
        }
        sleep(3);
        
        printf("\tret = %d\n\n", nbyte);
        
        puts("Received and wrote file data\n");
        
        // write success msg
        char result_msg[BUF_SIZE] = "Success!\n";    
        write(sd, result_msg, sizeof(result_msg));

        // close file
        fclose(fp);
    }

    close(sd);

    return 0;
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
}
