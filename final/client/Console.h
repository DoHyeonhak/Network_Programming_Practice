#define LEFT 68
#define RIGHT 67
#define UP 65
#define DOWN 66

void gotoxy(int x, int y);      // move cursor to (x, y) coordinate
void clrscr(void);              // clear screen

int kbhit();
int getch();
