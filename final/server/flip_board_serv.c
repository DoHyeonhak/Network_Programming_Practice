/*
    getitimer로 남은 시간 확인 방법 : https://blog.naver.com/skssim/121292037 
    getopt() : https://tamenut.tistory.com/entry/UNIX-getopt-%ED%95%A8%EC%88%98      
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "server.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex

int clnt_sock[LIMIT_CLNT];
int clnt_idx = 0;
int clnt_max = 0;

init_data_t basic_data;  // game basic data

board_t board[MAX_BOARD] = {0, 0};
int player_pos[MAX_USER] = {0};

int time_out = 0;        // 게임 종료 확인 변수

struct sigaction sa;
struct itimerval timer, get_time;

// functions
void* client_handler(void* arg);
void* center_handler(void* arg);
void set_timer();
void timer_handler(int signum);
void create_random_board();
void create_player_pos();
void print_arg_msg();
void error_handling(char *message);

int main(int argc, char *argv[]){

    pthread_t center_tid;
    pthread_t user_tid[LIMIT_CLNT];

    int result = 0;     // game result
    int UDP_PORT = 0;   // UDP port = TCP port + 1

    int c;
	extern char *optarg;
	extern int optind;

    char* n_value = NULL;
    char* s_value = NULL;
    char* b_value = NULL;
    char* t_value = NULL;
    char* p_value = NULL;

    while( (c=getopt(argc, argv, "n:s:b:t:p:")) != -1) {
		switch(c) {
			case 'n':
                n_value = optarg;
				break;

			case 's':
                s_value = optarg;
				break;

			case 'b':
                b_value = optarg;
				break;

			case 't':
                t_value = optarg;

			case 'p':
                p_value = optarg;
				break;
                
			default:
				break;
		}
	}

    // argument들이 모두 입력되었는지 확인
    if(n_value == NULL || s_value == NULL || b_value == NULL || t_value == NULL || p_value == NULL){
        print_arg_msg();
        return 1;
    }

    // Team balance를 위한 n, b가 짝수인지 확인
    if(atoi(n_value)%2 != 0){
        printf("n_value is not even number\n\n");
        return 1;
    }else if(atoi(b_value)%2 != 0){
        printf("b_value is not even number\n\n");
        return 1;
    }

    // update data
    basic_data.player_cnt = atoi(n_value);
    basic_data.player_id = 0;
    basic_data.grid_size = atoi(s_value);
    basic_data.board_cnt = atoi(b_value);
    basic_data.time = atoi(t_value);
    
    UDP_PORT = (atoi(p_value) + 1);          

    // TCP socket process
    int serv_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(p_value));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    clnt_max = atoi(n_value);   // backlog num
    if (listen(serv_sock, clnt_max) == -1)
        error_handling("listen() error");

    create_random_board();    // create board randomly
    create_player_pos();      // create user positison

    // write data and create thread
    while(clnt_idx < clnt_max){
        clnt_addr_size = sizeof(clnt_addr);
        printf("\nwait client\n");
        clnt_sock[clnt_idx] = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock[clnt_idx] == -1){
            error_handling("accept()");
            exit(1);
        }
        
        init_data_t data = basic_data;
        data.player_id = clnt_idx;    // 짝수면 red, 홀수면 blue

        write(clnt_sock[clnt_idx], &data, sizeof(data));
        write(clnt_sock[clnt_idx], &board, sizeof(board_t) * basic_data.board_cnt);
        write(clnt_sock[clnt_idx], &player_pos, sizeof(player_pos));

        pthread_create(&user_tid[clnt_idx], NULL, client_handler, (void*)&clnt_sock[clnt_idx]);
        clnt_idx++;
    }
    printf("\nconnect tcp, and create user threads\n");

    // count and timer
    sleep(3);                   // count time
    set_timer(basic_data.time); // start timer

    pthread_create(&center_tid, NULL, center_handler, (void*)&UDP_PORT);    // game start!

    
    while(time_out == 0); // wait

    // update score and send result to clients
    printf("\n[Result]\n");
    Result_t res = {0, 0};
    for(int i = 0; i < basic_data.board_cnt; i++){
        if(board[i].color == 1){    // Red board
            res.red ++;
        }else{                      // Blue board
            res.blue ++;
        }
    }

    // write result
    for(int i = 0; i < clnt_max; i++){
        write(clnt_sock[i], &res, sizeof(Result_t));
        printf("[%d] write result\n", clnt_sock[i]);
    }
    printf("\nwrite result to all clients\n\n");

    // close thread and socket
    for(int i = 0; i < clnt_max; i++){
        pthread_join(user_tid[i], NULL);
        close(clnt_sock[i]);
    }
    pthread_join(center_tid, NULL);
    close(serv_sock);
    
    printf("Bye!\n");

    return 0;
}

void* client_handler(void* arg){

    int sock = *((int*)arg);
    int board_cnt = basic_data.board_cnt;
    int player_cnt = basic_data.player_cnt;

    while(clnt_idx < clnt_max);     // 참여자들이 모두 접속하기 전까지 대기

    int start = 1;
    write(sock, &start, sizeof(int));
    printf("\n\t[%d] send start signal\n\n", sock);

    while(1){
        
        client_data_t data = {0,0};

        if(time_out){
            printf("\t[%d] client handler out\n", sock);
            return NULL;
        }

        int ret = read(sock, &data, sizeof(client_data_t));
        if(ret == -1){
            printf("read() error");
            exit(1);
        }

        pthread_mutex_lock(&mutex);
        player_pos[data.id] = data.position; 
        if(data.flag == 1){ // ENTER
            for(int i = 0; i < board_cnt; i++){
                // printf("\tu=%d, pan=%d\n", data.position, board[i].pos);
                if(data.position == board[i].pos){ // 판의 위치와 받은 위치가 같은 경우
                    printf("\t[%d] color=%d\n", i, board[i].color);
                    if(board[i].color == 1){
                        board[i].color = 0;
                    }else{
                        board[i].color = 1;
                    }
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void* center_handler(void* arg){ // UDP
    
    int port = *((int*)arg);
    int udp_sock;
    struct sockaddr_in mul_adr;
    int time_live = TTL;
    
    udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (udp_sock == -1)
    error_handling("UDP socket creation error");

    memset(&mul_adr, 0, sizeof(mul_adr));
    mul_adr.sin_family = AF_INET;
    mul_adr.sin_addr.s_addr = inet_addr(GROUP_IP);   // multicast IP
    mul_adr.sin_port = htons(port);                  // multicast port

    setsockopt(udp_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_live, sizeof(time_live));

    int board_cnt = basic_data.board_cnt;
    int player_cnt = basic_data.player_cnt;

    while(1){
        if(time_out){
            printf("\tcenter handler out\n");
            return NULL;
        }
        
        pthread_mutex_lock(&mutex);
        
        getitimer(ITIMER_VIRTUAL, &get_time);   // get current time
        
        UDP_packet_t packet;
        packet.sec = get_time.it_value.tv_sec;
        packet.usec = get_time.it_value.tv_usec;
        
        for(int i = 0; i < board_cnt; i++){
            packet.board_data[i] = board[i];
        }
        for(int i = 0; i < player_cnt; i++){
            packet.player_data[i] = player_pos[i];
        }
        
        sendto(udp_sock, &packet, sizeof(UDP_packet_t), 0, (struct sockaddr*)&mul_adr, sizeof(mul_adr));
        
        pthread_mutex_unlock(&mutex);
        // printf("\t[Time data]  sec = %ld, usec = %ld\n", packet.sec, packet.usec);
        usleep(5000);
    }

    close(udp_sock);
    return NULL;
}

void set_timer(int time){
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);
    
    timer.it_value.tv_sec = time;
    timer.it_value.tv_usec = 0;
    
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
}

void timer_handler(int signum){
    printf("\n\tTime over!\n\n");
    time_out = 1;
}

void create_random_board(){
    
    srand(time(NULL));
    
    int board_cnt = basic_data.board_cnt;
    int size = basic_data.grid_size;
    
    for(int i = 0; i < board_cnt; i++){
        int x = rand() % (size);
        int y = rand() % (size - 2) + 1;    // 상-하단 제외 (상-하단에 팀별 일렬 배치)
        int temp = x + y * size;
        int check = 0;  // 같은 것이 존재하는 경우에 1, 아니면 0
        for(int j = 0; j < i; j++){
            if(temp == board[j].pos){   
                check = 1;
                break;
            }
        }

        if(check){  // 같은 것이 존재하는 경우
            i--;
        }else{
            board[i].pos = temp;
            if((i%2) == 0){
                board[i].color = 1; // red    
            }else{
                board[i].color = 0; // blue
            }
        }
        printf("[%d] pos=%d, color=^%d\n", i, board[i].pos, board[i].color);
    }
}

void create_player_pos(){
    
    int size = basic_data.grid_size;
    int player_cnt = basic_data.player_cnt;
    
    int red_x = size - 1, red_y = 0;
    int blue_x = 0, blue_y = size - 1;
    
    for(int i = 0; i < player_cnt; i++){
        if((i % 2) == 0){ // red
            player_pos[i] = (red_x--) + (red_y) * size;
        }else{ // blue
            player_pos[i] = (blue_x++) + (blue_y) * size;
        }
    }
}

void print_arg_msg(){
    printf("Usage : ./game_serv -n [value] -s [value] -b [value] -t [value] -p [value] \n");
    printf("Option : \n");
	printf("\t-n : the number of players (only even number, maximum is 40)\n");
	printf("\t-s : grid size\n");
	printf("\t-b : the number of board (only even number, maximum is 8)\n");
    printf("\t-t : game progress time\n");
    printf("\t-p : server port\n");
}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
    exit(1);
}
