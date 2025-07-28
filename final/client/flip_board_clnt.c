#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>

#include "Console.h"
#include "client.h"

init_data_t basic_data;

board_t board[MAX_BOARD] = {0, 0};
int player_pos[MAX_USER] = {0};

int time_out = 0;   // check time out globally

int left_sec = 0;
int left_usec = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex

// function call
void* recv_handler(void* arg);
void* send_handler(void* arg);
void display_all(init_data_t data);


int main(int argc, char* argv[]){
    int ret = 0;
    init_data_t main_data;  // copy basic_data

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // TCP socket process
    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error!");    

    
    // UDP multicast socket process
    int udp_sock;
    struct sockaddr_in adr;
    struct ip_mreq join_adr;

    udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (udp_sock == -1)
        error_handling("socket() error");

    memset(&adr, 0, sizeof(adr));
    adr.sin_family=AF_INET;
    adr.sin_addr.s_addr=htonl(INADDR_ANY);
    adr.sin_port=htons(atoi(argv[2]));


    int flag;
    flag = fcntl(udp_sock, F_GETFL, 0);
    fcntl(udp_sock, F_SETFL, flag | O_NONBLOCK);    // non-blocking option

    // reuse option
    int optvalue = 1;
    if(setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue)) < 0){    // SO_REUSEADDR - multicast에서 필수
        error_handling("setsockopt(SO_REUSEADDR) failed");
    }
    
    if(bind(udp_sock, (struct sockaddr*)&adr, sizeof(adr)) == -1)
        error_handling("bind() error");

    join_adr.imr_multiaddr.s_addr=inet_addr(GROUP_IP);
    join_adr.imr_interface.s_addr=htonl(INADDR_ANY);

    setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join_adr, sizeof(join_adr));

    // read basic data
    read(sock, &main_data, sizeof(main_data));  
    basic_data = main_data;

    read(sock, &board, sizeof(board_t) * basic_data.board_cnt);
    read(sock, &player_pos, sizeof(player_pos));
    
    
    display_wait_n_count(main_data, sock);

    // thread 생성
    pthread_t send_tid;
    pthread_t recv_tid;

    pthread_create(&send_tid, NULL, send_handler, (void*)&sock);       // tcp 
    pthread_create(&recv_tid, NULL, recv_handler, (void*)&udp_sock);   // udp

    // Console
    clrscr();

    while(1){

        if(time_out) break;
        
        display_all(main_data);

        usleep(5000);        
    
    }

    Result_t res = {0, 0};
    read(sock, &res, sizeof(Result_t));

    display_result(main_data, res);
    

    pthread_join(send_tid, NULL);
    pthread_join(recv_tid, NULL);

    printf("Bye!\n");

    close(sock);
    close(udp_sock);
    return 0;
}

void* recv_handler(void* arg){
    
    int sock = *((int*)arg);
    int ret = 0;

    int board_cnt = basic_data.board_cnt;
    int player_cnt = basic_data.player_cnt;

    while(1){

        if(time_out){
            return NULL;
        }
        
        UDP_packet_t packet;

        ret = recvfrom(sock, &packet, sizeof(UDP_packet_t), 0, NULL, 0);
        
        pthread_mutex_lock(&mutex);
        for(int i = 0; i < board_cnt; i++){
            board[i] = packet.board_data[i];
        }
        for(int i = 0; i < player_cnt; i++){
            player_pos[i] = packet.player_data[i];
        }

        left_sec = (int)packet.sec;
        left_usec = (int)packet.usec;

        pthread_mutex_unlock(&mutex);


        if(packet.sec == 0 && packet.usec <= 7000){
            time_out = 1;
            break;
        }

    }
    
    return NULL;
}

void* send_handler(void* arg){
    
    int sock = *((int*)arg);

    int size = basic_data.grid_size;
    int id = basic_data.player_id;
    int board_cnt = basic_data.board_cnt;

    while(1){

        if(time_out) return NULL;

        client_data_t player = {id, player_pos[id], 0};

        int x = player.pos % size;
        int y = player.pos / size;

        int flag = 0;

        if(kbhit()){
            int key = getch();

            if(key == ENTER){
                flag = 1;
            }else if(key == '['){
                key = getch();
                switch(key){
                    case LEFT:   // left
                        x = ((x - 1) < 0 ? 0 : (x - 1));
                    break;
                    
                    case RIGHT:   // right
                        x = ((x + 1) > (size - 1) ? x: (x + 1));
                        break;
        
                    case UP:   // up
                        y = ((y - 1) < 0 ? y : (y - 1));
                        break;
        
                    case DOWN:   // down
                        y = ((y + 1) > (size - 1) ? y : (y + 1));
                        break;
        
                    default:
                        break;
                }
            }

            // 데이터 가공 및 전송
            pthread_mutex_lock(&mutex);
            player_pos[id] = x + y * size;
            pthread_mutex_unlock(&mutex);

            player.pos = x + y * size;
            player.flag = flag;

            write(sock, &player, sizeof(client_data_t));
        }
    }
    
    return NULL;
}

void display_all(init_data_t data){

        pthread_mutex_lock(&mutex);

        printf("\e[?25l");  // hide cursor

        gotoxy(1, 2);
        printf("                       ");
        gotoxy(1, 2);
        if(left_sec == 0 && left_usec <= 7000){
            printf("Time out");
        }else{
            printf("Time: %d.%d sec\n", left_sec, left_usec);
        }

        gotoxy(1, 3);
        for(int i = 0; i < data.grid_size; i++){
            printf("\033[38;2;255;203;97m");
            printf("- ");
            printf("\033[0m");
        }
        
        // 1. 그리드 전체 좌표마다 position을 비교하여 출력
        gotoxy(1, 4);
        for(int i = 0; i < data.grid_size; i++){
            for(int j = 0; j < data.grid_size; j++){
                int temp = j + i * data.grid_size;
                int print_check = 0;

                // 츨력 레이어
                // 1.플레이어 자신, 2.다른 플레이어, 3.판넬, 4.공백
                if(temp == player_pos[data.player_id]){ // 해당 위치가 클라이언트 자신의 위치와 같은 경우
                    if(data.player_id%2 == 0){
                        printf("\033[38;2;252;127;0m");
                        printf("R ");
                        printf("\033[0m");
                    }else{
                        printf("\033[38;2;0;255;255m");
                        printf("B ");
                        printf("\033[0m");    
                    }
                    print_check = 1;
                }
                if(print_check == 1){
                    continue;
                }

                for(int k = 0; k < data.player_cnt; k++){ // 플레이어 출력
                    if(temp == player_pos[k]){
                        if(k%2 == 0){               // red
                            printf("\033[38;2;234;91;111m");
                            printf("U ");
                            printf("\033[0m");
                        }else{                      // blue
                            printf("\033[38;2;119;190;240m");
                            printf("U ");
                            printf("\033[0m");    
                        }

                        print_check = 1;
                        break;
                    }
                }
                if(print_check == 1){  // 플레이어가 출력된 경우 continue
                    continue;
                }
                
                for(int k = 0; k < data.board_cnt; k++){ // 판 출력
                    if(temp == board[k].pos){
                        if(board[k].color == 1){    // red
                            printf("\033[38;2;234;91;111m");
                            printf("■ ");
                            printf("\033[0m");
                        }else{                      // blue
                            printf("\033[38;2;119;190;240m");
                            printf("■ ");
                            printf("\033[0m");
                        }
                        print_check = 1;
                        break;
                    }
                }
                if(print_check == 1){   // 판이 출력된 경우 continue
                    continue;
                }

                printf("  ");   // 아무것도 출력되지 않은 경우 공백 출력
            }
            putchar('\n');
        }

        gotoxy(1, 4 + data.grid_size);
        for(int i = 0; i < data.grid_size; i++){
            printf("\033[38;2;255;203;97m");
            printf("- ");
            printf("\033[0m");
        }

        fflush(stdout);
        printf("\e[?25h");  // show cursor
        pthread_mutex_unlock(&mutex);

}