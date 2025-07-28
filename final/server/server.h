#define BUF_SIZE 30
#define LIMIT_CLNT 8
#define MAX_GRID_SIZE 30
#define MAX_USER 8
#define MAX_BOARD 40

// for UDP
#define TTL 64  // time to live : 패킷 생존 시간
#define GROUP_IP "224.1.1.2"

// structure
typedef struct{
    int player_cnt;
    int player_id;  // 짝수는 red, 홀수는 blue
    int grid_size;  // -s로 받은 그리드 한 변의 크기
    int board_cnt;  // -b로 받은 판 개수
    int time;       // 게임 시간
}init_data_t;

typedef struct{
    int id;
    int position;    // “int” type, for bitwise operation
    int flag;        // 이동인지 뒤집기인지 (enter 눌렀을 때 1, 아니면 0)
}client_data_t;

typedef struct{
    int pos;
    int color;
}board_t;

typedef struct{
    long sec;
    long usec;
    board_t board_data[MAX_BOARD];
    int player_data[MAX_USER];
}UDP_packet_t;

typedef struct{
    int red;
    int blue;
}Result_t;