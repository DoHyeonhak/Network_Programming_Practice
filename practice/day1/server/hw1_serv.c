#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <dirent.h>		// opendir, readdir.etc.
#include <sys/stat.h>	// stat, mkdir
#include <errno.h>		// check error

#define BUF_SIZE 30
#define LIST_SIZE 32

typedef struct{
    char f_name[32];
    long f_size;
}FILE_IN_DIR;

FILE_IN_DIR files[LIST_SIZE];   // list variable
int cnt = 0;    // list counting variable

char msg[2048] = "";

void error_handling(char *message);
void read_list(int clnt_sd);

int main(int argc, char *argv[])
{

    int serv_sd, clnt_sd;
    FILE * fp;
    char filename[BUF_SIZE];    // file name
    int read_len = 0;
    int ret = 0;
    long filesize = 0;

    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // socket
    serv_sd = socket(PF_INET, SOCK_STREAM, 0);

    // bind
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
    bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    // listen
    listen(serv_sd, 5);

    // accept
    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
    
    // program loop
    while(1){
        // check the progress about program
        char progress[BUF_SIZE];
        read_len = read(clnt_sd, &progress, BUF_SIZE);   // read signal
        if (read_len == -1){
            error_handling("read() error!");
            exit(1);
        }
        if(strcmp(progress, "exit") == 0){  // exit program 
            printf("Bye!\n");
            break;
        }
            
        read_list(clnt_sd); // call for reading list data writing them to client
        // display list to check
        printf("[LIST]\n");
        for(int i = 0; i < cnt; i++){
            printf("%-16s | %ld\n", files[i].f_name, files[i].f_size);   // print file info (name, size)
        }
        putchar('\n');
        
        // int read_len = 0;
        
        // read file name
        read_len = read(clnt_sd, &filename, BUF_SIZE);
        if (read_len == -1){
            error_handling("read() error!");
            exit(1);
        }
        printf("Received file name: %s \n", filename);
    
        
        // file open
        fp = fopen(filename, "rb");
        if(fp == NULL) {
            printf("file open error\n");
            exit(1);
        }

        // find file
        for(int i = 0; i < cnt; i++){ // 요청한 파일에 맞는 파일 사이즈를 찾아서 작업
            if(strcmp(filename, files[i].f_name) == 0){
                filesize = files[i].f_size;
                break;
            }
        }

        char data[filesize];    // variable about the chosen file data
        data[0] = '\0'; // initialize
        
        // read file data
        fread((void*)data, 1, filesize, fp);

        // printf("\n\tfile_data = \n%s\n", data);
        printf("\tdata_size = %ld\n", sizeof(data));
        
        // write file data to client
        write(clnt_sd, data, filesize);   // 파일 데이터 보내기
        
        printf("Send file\n");

        // read and wait success msg
        char result_msg[BUF_SIZE];
        read(clnt_sd, &result_msg, BUF_SIZE);
        printf("%s\n", result_msg);

        // close file
        fclose(fp);
    }

    // close sockets
    close(clnt_sd); close(serv_sd);

    return 0; 
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
}

void read_list(int clnt_sd){

    msg[0] = '\0'; // initialize for loop
    cnt = 0; // initialize for loop


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
 
        // assign file data and combine data to one string
        strcpy(files[cnt].f_name, entry->d_name);
        files[cnt].f_size = st.st_size;   

        char temp[64];
        sprintf(temp, "%-18s %ld\n", files[cnt].f_name, files[cnt].f_size);
        strcat(msg, temp);
        
        cnt++;  // increase list index
	}
    
    write(clnt_sd, &msg, sizeof(msg));  // write all list data. one string.

	// close dir
	closedir(dir);
}