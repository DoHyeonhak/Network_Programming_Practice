#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "Console.h"


void display_wait_n_count(init_data_t data, int sock){
    
    clrscr();
    gotoxy(1, 1);    // y, x
    for(int i = 0; i < data.grid_size; i++){
        printf("- ");
    }
    putchar('\n');

    gotoxy(data.grid_size - 7, data.grid_size/2);
    printf("Flipping Board!");
    gotoxy(data.grid_size -5, data.grid_size/2 + 2);
    printf("Waiting...");

    gotoxy(1, 2 + data.grid_size);
    for(int j = 0; j < data.grid_size; j++){
        printf("- ");
    }
    fflush(stdout);
    
    int start_signal = 0;
    read(sock, &start_signal, sizeof(int));  // 기본 정보

    clrscr();
    for(int i = 3; i > 0; i--){
        gotoxy(1, 1);    // y, x
        for(int i = 0; i < data.grid_size; i++){
            printf("- ");
        }
        putchar('\n');

        gotoxy(data.grid_size - 3, data.grid_size/2 + 1);
        printf("Count %d", i);

        gotoxy(1, 2 + data.grid_size);
        for(int j = 0; j < data.grid_size; j++){
            printf("- ");
        }
        fflush(stdout);
        sleep(1);
    }

}

void display_result(init_data_t data, Result_t result){
    char msg[10] = "";
    int score = result.red - result.blue;
    
    clrscr();
    
    gotoxy(1, 1);
    for(int i = 0; i < data.grid_size; i++){
        printf("\033[38;2;255;203;97m");
        printf("- ");
        printf("\033[0m");
    }
    putchar('\n');

    gotoxy(data.grid_size - 5, data.grid_size/2 - 1);
    printf("Time Over!");
    if(score > 0){
        gotoxy(data.grid_size - 6, data.grid_size/2 + 1);
        printf("\033[38;2;234;91;111m");
        printf("Red Team Win!");
        printf("\033[0m");
    }else if(score < 0){
        gotoxy(data.grid_size - 7, data.grid_size/2 + 1);
        printf("\033[38;2;119;190;240m");
        printf("Blue Team Win!");
        printf("\033[0m");
    }else{
        gotoxy(data.grid_size - 2, data.grid_size/2 + 1);
        printf("\033[38;2;13;188;121m");
        printf("Draw!");
        printf("\033[0m");
    }

    gotoxy(data.grid_size - 8, data.grid_size/2 + 3);
    printf("Red: %2d Blue: %2d", result.red, result.blue);

    gotoxy(1, 2 + data.grid_size);
    for(int i = 0; i < data.grid_size; i++){
        printf("\033[38;2;255;203;97m");
        printf("- ");
        printf("\033[0m");
    }
    fflush(stdout);
    sleep(1);

}

void error_handling(char *message){
    printf("ERROR: %s\n", message);
    exit(1);
}