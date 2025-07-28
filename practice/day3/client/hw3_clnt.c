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
    char f_type[10];
    long f_size;
}FILE_IN_DIR;

FILE_IN_DIR files[LIST_SIZE];   // list variable
int cnt = 0;                    // list counting variable

void error_handling(char *message);
void print_list();
int search_file(char* ptr);
int search_dir(char* ptr);

int main(int argc, char *argv[])
{
    int sd;
    FILE *fp;
    
    char buf[BUF_SIZE];
    int read_cnt;
    struct sockaddr_in serv_adr;
    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));
    
    connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    printf("Connected!\n\n");

    // read list data
    read(sd, &cnt, sizeof(int));
    int i = 0;
    while(i < cnt){
        int read_len = read(sd, &files[i], sizeof(FILE_IN_DIR));
        i++;
    }

    printf("Welcome to shell!\n");
	printf(" - cd <dir>: change directory\n");
	printf(" - ls: list the files and directories\n");
    printf(" - down <filename>: download <filename> in the current directory \n");
    printf(" - upload <filename>: upload <filename> to the server directory \n");
	printf(" - exit: print  \"Bye!\"  and terminate shell\n");
    printf("\n");

    int len = 0;
    while(1){
        char cmd[64] = "";
        printf("shell $ ");
        
        fgets(cmd, 64, stdin);
        cmd[strlen(cmd)-1] = '\0';
    
        write(sd, cmd, strlen(cmd));

        char* cmd_ptr = strtok(cmd, " ");       // for cmd
		char* cmd_ptr2 = strtok(NULL, "");      // for dir name and file name

        // mode
        if(strcmp(cmd_ptr, "cd")==0){
            int ret = search_dir(cmd_ptr2);
            if(ret == -1){
                printf("Not a directory.\n");
                continue;
            }
            cnt = 0; 
            read(sd, &cnt, sizeof(int));

            int i = 0;
            while(i < cnt){
                int read_len = read(sd, &files[i], sizeof(FILE_IN_DIR));
                i++;
            }

        }else if(strcmp(cmd_ptr, "ls") == 0){
            cnt = 0; 
            read(sd, &cnt, sizeof(int));

            int i = 0;
            while(i < cnt){
                int read_len = read(sd, &files[i], sizeof(FILE_IN_DIR));
                i++;
            }
            print_list();
        }else if(strcmp(cmd_ptr, "down") == 0){
            int ret =  search_file(cmd_ptr2);
            if(ret == -1){
                printf("Not founded.\n\n");
                continue;
            }

            long size = 0;
            read(sd, &size, sizeof(long));

            fp = fopen(cmd_ptr2, "wb");     
        
            // read file
            char buffer[256] = "";
            int nbyte = 0;
            int read_len = 0;
            while(nbyte < size){
                read_len = read(sd, &buffer, 256);
                if (read_len == -1){
                    error_handling("read() error!");
                    exit(1);
                }
                nbyte += read_len;
                fwrite(buffer, 1, read_len, fp);
            }
            puts("Received file data");

            fclose(fp);

        }else if(strcmp(cmd_ptr, "upload") == 0){

            fp = fopen(cmd_ptr2, "rb");
            
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            
            write(sd, &size, sizeof(long));

            fseek(fp, 0, SEEK_SET); // reset file pointer
            char data[size];
            data[0] = '\0';

            fread((void*)data, 1, size, fp);
            // printf("%s\n", data);
            write(sd, data, sizeof(data));

            puts("Uploaded file data");
            fclose(fp);
        }else if(strcmp(cmd_ptr, "exit")==0){
            printf("\nBye!\n");
            break;
        }else{
            printf("\nIncorrect Command..\n\n");
        }
    }
    
    close(sd);
    return 0;
}

void print_list(){
    putchar('\n');
    for(int i = 0; i < cnt; i++){
        printf("%-24s | %ld\n", files[i].f_name, files[i].f_size);
    }
    putchar('\n');
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
}

int search_file(char* ptr){
    int ret = -1;
    for(int i = 0; i < cnt; i++){
        if(strcmp(ptr, files[i].f_name) == 0){
            ret = i;
            break;
        }
    }
    return ret;
}

int search_dir(char* ptr){
    int ret = -1;
    for(int i = 0; i < cnt; i++){
        if((strcmp(ptr, files[i].f_name) == 0) && (strstr(files[i].f_type, "dir") > 0)){
            ret = i;
            break;
        }
    }
    return ret;
}