/*
    Based on 2025-1 Operating System Korean
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>

void clrscr();
int getch();
void gotoxy(int x,int y);

