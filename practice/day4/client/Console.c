/*
    Based on 2025-1 Operating System Korean
*/

#include "Console.h"

int getch(){
	int c = 0;
	struct termios oldattr, newattr;

	tcgetattr(STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	newattr.c_cc[VMIN] = 1;
	newattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
	c = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

	return c;
}

void gotoxy(int x,int y){
    printf("%c[%d;%df",0x1B,y,x);
}

void clrscr(){
	fprintf(stdout, "\033[2J\033[0;0f");
	fflush(stdout);
}
        