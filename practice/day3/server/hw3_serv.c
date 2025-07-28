/*
    https://ethical-hack.tistory.com/78        : getcwd()
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define BUF_SIZE 4
#define EPOLL_SIZE 50
#define LIST_SIZE 32

typedef struct{
    char f_name[32];
    char f_type[10];
    long f_size;
}FILE_IN_DIR;

// global variable
FILE_IN_DIR files[LIST_SIZE];
int cnt = 0;

char clnt_path[EPOLL_SIZE][256];

void read_save_list(int clnt_sd);
int search_file(char* ptr);
void error_handling(const char *msg);
void send_file(int sock, int idx);
void download_file(int sock, char* filename);
void send_list(int sock);

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    
    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;
    
    char buf[256];
    int str_len = 0;
    int i = 0;

    FILE* fp = NULL;
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // socket
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    // bind and listen
    bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    listen(serv_sock, 5);

    // epoll process
    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    char default_path[256];     // 현재 서버 파일이 있는 경로를 default로 저장
    getcwd(default_path, 256);
    // printf("path=%s\n", default_path);

    while(1){
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if(event_cnt == -1){
            puts("epoll_wait() error");
            break;
        }

        puts("return epoll_wait");

        for(i = 0; i < event_cnt; i++){
            if(ep_events[i].data.fd == serv_sock){
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);

                event.events = EPOLLIN;  
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);

                strcpy(clnt_path[clnt_sock], default_path);
                read_save_list(clnt_sock);
                send_list(clnt_sock);
                
                printf("connected client: %d \n", clnt_sock);
            }else{
                str_len = read(ep_events[i].data.fd, buf, sizeof(buf));
                buf[str_len] = '\0';
                char* cmd_ptr = strtok(buf, " ");       // command
		        char* cmd_ptr2 = strtok(NULL, "\0");    // dir, file name, or space

                if(str_len == 0){  // close request
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("closed client: %d \n", ep_events[i].data.fd);

                }else{
                    if(strcmp(cmd_ptr, "cd") == 0){
                        printf("\n[cd]\n");
                        int ret = search_file(cmd_ptr2);
                        if(ret == -1){  // empty
                            printf("Not founded.\n");

                        }else if(strcmp(files[ret].f_type, "dir") != 0){ // not dir
                            printf("Not a directory.\n");

                        }else{  // dir
                            chdir(cmd_ptr2);  // change direcory : 해당 파일이 있는 전체에 영향을 줌. 경로 저장 필요
                            getcwd(clnt_path[ep_events[i].data.fd], 256);

                            read_save_list(ep_events[i].data.fd);
                            send_list(ep_events[i].data.fd);
    
                            puts("Changed directory\n");
                        }
                    }else if(strcmp(cmd_ptr,"ls") == 0){
                        chdir(clnt_path[ep_events[i].data.fd]);
                        
                        printf("fd=%d, path=%s\n", ep_events[i].data.fd, clnt_path[ep_events[i].data.fd]);

                        read_save_list(ep_events[i].data.fd);       // 갱신
                        send_list(ep_events[i].data.fd);            // 갱신
                        
                        printf("\n[ls] - only client work\n\n");
                    }else if(strcmp(cmd_ptr, "down") == 0){
                        printf("\n[down]\n");
                        int ret = search_file(cmd_ptr2);
                        if(ret == -1){
                            printf("Not founded.\n\n");
                        }else{
                            send_file(ep_events[i].data.fd, ret);
                        }
                    }else if(strcmp(cmd_ptr, "upload") == 0){
                        printf("\n[upload]\n");
                        download_file(ep_events[i].data.fd, cmd_ptr2);

                    }
                }
            }
        }
    }

    close(serv_sock);
    close(epfd);
    free(ep_events);
    return 0;
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

void read_save_list(int clnt_sd){
    cnt = 0;
    char* path = clnt_path[clnt_sd];
	
	DIR *dir = opendir(path);
	if(!dir){
        printf("%s\n", strerror(errno));
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

		struct stat st;
		if(stat(full_path, &st)==-1){
			printf("stat error");
			exit(1);
        }

        strcpy(files[cnt].f_name, entry->d_name);
        files[cnt].f_size = st.st_size;   
        if(entry->d_type == DT_REG){
            strcpy(files[cnt].f_type, "file");
        }else{
            strcpy(files[cnt].f_type, "dir");
        }
        // printf("%-24s | %s | %ld\n", files[cnt].f_name, files[cnt].f_type, files[cnt].f_size);
        cnt++;  // increase list index
	}
    
	closedir(dir);
}

void send_list(int sock){
    write(sock, &cnt, sizeof(int));
    for(int i = 0; i < cnt; i++){
        write(sock, &files[i], sizeof(FILE_IN_DIR));
    }
}

void error_handling(const char *msg) {
    perror(msg); exit(1);
}

void send_file(int sock, int idx){
    printf("%s %ld\n", files[idx].f_name, files[idx].f_size);
    write(sock, &files[idx].f_size, sizeof(long));
                        
    char data[files[idx].f_size];
    data[0] = '\0';
                        
    FILE* fp = fopen(files[idx].f_name, "rb");

    fseek(fp, 0, SEEK_SET);

    fread((void*)data, 1, files[idx].f_size, fp);
    write(sock, data, sizeof(data));

    fclose(fp);

    puts("Sent file\n");
}

void download_file(int sock, char* filename){
    long size = 0;
    read(sock, &size, sizeof(long));
    printf("filename=%s, size=%ld\n", filename, size);

    char path[256] = "";
    strcpy(path, clnt_path[sock]);
    strcat(path, "/");
    strcat(path, filename);

    FILE* fp = fopen(path, "wb");

    char buffer[256] = "";
    int nbyte = 0;
    int read_len = 0;
    while(nbyte < size){
        read_len = read(sock, &buffer, 256);
        if (read_len == -1){
            error_handling("read() error!");
            exit(1);
        }
        nbyte += read_len;
        fwrite(buffer, 1, read_len, fp);
    }
    puts("Received file data\n");

    fclose(fp);
}