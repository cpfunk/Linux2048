/* 2048 for Linux terminal - Based on 2048 by Gabriele Cirulli
 * 
 * Written by:
 * Christopher Funk
 */

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define WHITE 0
#define RED 1
#define BLUE 2
#define CYAN 3
#define GREEN 4
#define YELLOW 5
#define MAGENTA 6

#define GRID_H(m) (4 * m + 1)
#define GRID_W(n) (8 * n + 1)

void zeroVarArray(int m, int n, int matrix[][n]);
void drawGrid(WINDOW *win, int m, int n, int matrix[][n]);
void fillAnEmptySquare(int m, int n, int matrix[][n]);
void drawMatrix(WINDOW *win, int m, int n, int matrix[][n]);
int invalidKey(int keypress);
void shiftGrid(int keypress, int* score, int m, int n, int matrix[][n]);
int gameOver(int m, int n, int matrix[][n]);
int mostSigDigitIndex(int num);
int zeroCount(int m, int n, int matrix[][n]);

typedef struct {
	int x;
	int y;
} dimension;

int main(int argc, char **argv) {
	int gameIsOver = 0;
	//game matrix variables
	int usrSize = 0;
	int m = 4, n = 4;
	//create struct to store terminal window dimensions
	struct winsize terminalSize;
	//misc. dimensions variables
	dimension scoreMargin, gameOverMargin;
	//default window dimensions
	dimension winSize = {80, 24};
	
	//get terminal window dimensions
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminalSize);
	
	//if terminal window is larger than default then scale ncurses window to fit terminal
	if (winSize.y < terminalSize.ws_row) {
		winSize.y = terminalSize.ws_row;
	}
	if (winSize.x < terminalSize.ws_col) {
		winSize.x = terminalSize.ws_col;
	}
	//variable to store keyboard input
	int keypress;
	int score = 0;
	
	//user may specify a custom grid size
	if (argc == 2) sscanf(argv[1], "%d", &usrSize);
	
	if(usrSize > 1 && GRID_W(usrSize) < winSize.x && GRID_H(usrSize) + 3 < winSize.y && usrSize < 11) {
		m = usrSize;
		n = usrSize;
	}
	//game grid dimensions
	dimension gameGridSize = {GRID_W(n), GRID_H(m)};
	
	//margins for GAME OVER text placement
	gameOverMargin.y = gameGridSize.y / 2;
	gameOverMargin.x = (gameGridSize.x - strlen(" GAME OVER ")) / 2;
	
	int gameMatrix[m][n];
	//populate variable-sized matrix with zeros
	zeroVarArray(m, n, gameMatrix);
	
	//start curses mode
	initscr();
	//enable ^C exit
	cbreak();
	//hide keyboard input
	noecho();
	//make curser invisible
	curs_set(0);
	
	//check for color support
	if (has_colors() == 0) {
    endwin();
    printf("Terminal does not support color");
    exit(-1);
	}
	
	//start ncurses color
	start_color();
	//initialize color pairs
	init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
	init_pair(RED, COLOR_RED, COLOR_BLACK);
	init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
	init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
	init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
	init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
	
	//create ncurses window
	WINDOW *winMain = newwin(winSize.y, winSize.x, 0, 0);
	//create game window and place in center of window
	WINDOW *winGame = newwin(gameGridSize.y, gameGridSize.x, (winSize.y - gameGridSize.y) * 0.5, (winSize.x - gameGridSize.x) * 0.5);
	//print window on to the real screen
	refresh();
	
	//draw main border
	box(winMain, 0, 0);
	//print game title
	mvwprintw(winMain, 0, (winSize.x - strlen(" 2048 ")) * 0.5, " 2048 ");
	//draw game grid
	drawGrid(winGame, m, n, gameMatrix);
	//fill two random spots with numbers
	for (int i = 0; i < (m^2)/4; i++) {
		fillAnEmptySquare(m, n, gameMatrix);
	}
	//populate game grid
	drawMatrix(winGame, m, n, gameMatrix);
	//display scoreboard
	scoreMargin.y = (winSize.y - gameGridSize.y) / 2 - 1;
	scoreMargin.x = (winSize.x - gameGridSize.x) / 2;
	mvwprintw(winMain, scoreMargin.y, scoreMargin.x, "Score: %d", score);
	mvwprintw(winMain, winSize.y - 2, 1, " (Q)Exit");
	//update windows
	wrefresh(winMain);
	wrefresh(winGame);
	
	//enter gameloop
	while (1) {
		//wait for keyboard input
		keypress = getch();
		
		if (keypress == 'q' || keypress == 'Q') {
			break;	
		} else if (invalidKey(keypress) || gameIsOver) {
			continue;
		}
		//takes arrow key input and transforms the matrix appropriately
		shiftGrid(keypress, &score, m, n, gameMatrix);
		//check for end of game
		
		//update game grid with new numbers
		drawMatrix(winGame, m, n, gameMatrix);
		
		gameIsOver = gameOver(m, n, gameMatrix);
		if (gameIsOver) {
			wattron(winGame, COLOR_PAIR(RED));
			mvwprintw(winGame, gameOverMargin.y, gameOverMargin.x, " GAME OVER ");
			wattroff(winGame, COLOR_PAIR(RED));
		}
		//update scoreboard
		mvwprintw(winMain, scoreMargin.y, scoreMargin.x, "Score: %d", score);
		//update display
		wrefresh(winMain);
		wrefresh(winGame);
	}
	
	//end curses mode
	endwin();
	
	return 0;
}

void matrixCpy(int m, int n, int cpy[][n], int matrix[][n]) {
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			cpy[i][j] = matrix[i][j];
		}
	}
}

void hMirror(int m, int n, int matrix[][n]) {
	int temp;
	int end = m - 1;
	
	for (int i = 0; i < m / 2; i++) {
		for (int j = 0; j < n; j++) {
			temp = matrix[i][j];
			matrix[i][j] = matrix[end - i][j];
			matrix[end - i][j] = temp;
		}
	}
}

void rotateLeft(int m, int n, int matrix[][n]) {
	int temp[m][n];
	int end = m - 1;
	
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			temp[end - j][i] = matrix[i][j];
		}
	}
	matrixCpy(m, n, matrix, temp);
}

void rotateRight(int m, int n, int matrix[][n]) {
	int temp[m][n];
	int end = n - 1;
	
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			temp[j][end - i] = matrix[i][j];
		}
	}
	matrixCpy(m, n, matrix, temp);
}

void shiftColUpToRow(int col, int shiftUpTo, int m, int n, int matrix[][n]) {
	for (int row = shiftUpTo; row < m; row++) {
		if (row != m - 1) {
			matrix[row][col] = matrix[row + 1][col];
		} else {
			//pad bottom row with zero
			matrix[row][col] = 0;
		}
	}
}

void sumAndShiftUp(int* score, int m, int n, int matrix[][n]) {
	//array used to store the number of non-zero elements in each column
	int* valCount = calloc(m, sizeof(int));
	short didShift = 0, didSum = 0;
	short newSquares;
	
	//NULL check
	if (valCount == NULL) {
		endwin();
		perror("memory allocation for array shiftsAt failed");
		exit(-1);
	}
	
	//shift elements in each column to top
	for (int col = 0; col < n; col++) {
		for (int row = 0; row < m; row++) {
			if (matrix[row][col] != 0) {
				//only shift the elements up if they are not already at top
				if (row > valCount[col]) {
					matrix[valCount[col]][col] = matrix[row][col];
					matrix[row][col] = 0;
					didShift = 1;
				}
				valCount[col]++;
			}
		}
	}
	//check for sums
	for (int col = 0; col < n; col++) {
		for (int row = 0; row < valCount[col] - 1; row++) {
			//if element below is equal to current element
			if (matrix[row][col] == matrix[row + 1][col]) {
				//add to current element 
				matrix[row][col] *= 2;
				//shift all elements below up by 1 and pad with 0
				shiftColUpToRow(col, row + 1, m, n, matrix);
				*score += matrix[row][col];
				didSum = 1;
			}
		}
	}
	
	if (didShift || didSum) {
		if (m < 4) {
			newSquares = 1;
		} else {
			newSquares = m/4;
		}
		for (int i = 0; i < newSquares; i++) {
			fillAnEmptySquare(m, n, matrix);
		}
	}
	// free allocated memory
	free(valCount);
}

void shiftGrid(int keypress, int* score, int m, int n, int matrix[][n]) {
	enum key {
		UP = 65,
		DOWN = 66,
		LEFT = 68,
		RIGHT = 67
	};
	
	switch (keypress) {
		case UP:
			sumAndShiftUp(score, m, n, matrix);
			break;
		case DOWN:
			hMirror(m, n, matrix);
			sumAndShiftUp(score, m, n, matrix);
			hMirror(m, n, matrix);
			break;
		case LEFT:
			rotateRight(m, n, matrix);
			sumAndShiftUp(score, m, n, matrix);
			rotateLeft(m, n, matrix);
			break;
		case RIGHT:
			rotateLeft(m, n, matrix);
			sumAndShiftUp(score, m, n, matrix);
			rotateRight(m, n, matrix);
			break;
	}
}

int invalidKey(int keypress) {
	int validKeys[] = {65, 66, 68, 67};
	int length = sizeof(validKeys) / sizeof(int);
	
	for (int i = 0; i < length; i++) {
		if (keypress == validKeys[i]) {
			return 0;
		}
	}
	
	return 1;
}


int gameOver(int m, int n, int matrix[][n]) {
	int notAtBottom, notAtRight;
	int valBelow, valRight;
	
	if (zeroCount(m, n, matrix) != 0) return 0;
	
	for (int row = 0; row < m; row++) {
		for (int col = 0; col < n; col++) {
			notAtBottom = (row != m - 1);
			notAtRight = (col != n - 1);
			
			//some of these values will be 0 if the index is at the matrix's edge
			//if the current index's value is zero, then the game is certainly not over
			valBelow = notAtBottom * matrix[row + 1][col];
			valRight = notAtRight * matrix[row][col + 1];
			
			if (matrix[row][col] == valBelow) return 0;
			if (matrix[row][col] == valRight) return 0;
		}
	}
	
	return 1;
}


void drawGrid(WINDOW *win, int m, int n, int matrix[][n]) {
	int gridHeight = GRID_H(m);
	int gridWidth = GRID_W(n);
	
	for (int row = 0; row < gridHeight; row++) {
		for (int col = 0; col < gridWidth; col++) {

			//print horizontal gridlines
			if (!(row % 4))
				mvwaddch(win, row, col, ACS_HLINE);
			
			//print vertical gridlines
			if (!(col % 8))
				mvwaddch(win, row, col, ACS_VLINE);
			
			//print gridline intersections
			if (!(row % 4) && !(col % 8))
				mvwaddch(win, row, col, ACS_PLUS);
			
			//print border gridline intersections
			if (col == 0 && !(row % 4))
				mvwaddch(win, row, col, ACS_LTEE);
			else if (row == 0 && !(col % 8))
				mvwaddch(win, row, col, ACS_TTEE);
			else if (row == gridHeight - 1 && !(col % 8))
				mvwaddch(win, row, col, ACS_BTEE);
			else if (col == gridWidth - 1 && !(row % 4))
				mvwaddch(win, row, col, ACS_RTEE);

			//print corners
			if (row == 0 && col == 0)
				mvwaddch(win, row, col, ACS_ULCORNER);
			else if (row == 0 && col == gridWidth - 1)
				mvwaddch(win, row, col, ACS_URCORNER);
			else if (row == gridHeight - 1 && col == 0)
				mvwaddch(win, row, col, ACS_LLCORNER);
			else if (row == gridHeight - 1 && col == gridWidth - 1)
				mvwaddch(win, row, col, ACS_LRCORNER);
		}
	}
}

//chooses a color based on the element's value
int pickElementColor(int val) {
	if(val < 64) {
		return WHITE;
	} else if(val == 64) {
		return BLUE;
	} else if (val == 128) {
		return CYAN;
	} else if (val == 256) {
		return GREEN;
	} else if (val == 512) {
		return YELLOW;
	} else if (val == 1024) {
		return MAGENTA;
	} else if (val > 1024) {
		return RED;
	}
	//default
	return WHITE;
}


void drawMatrix(WINDOW *win, int m, int n, int matrix[][n]) {
	dimension numPosition;
	dimension spacePosition;
	int color;
	
	for (int row = 0; row < m; row++) {
		for (int col = 0; col < n; col++) {
			spacePosition.y = 4 * row + 2;
			spacePosition.x = 8 * col + 1;
			mvwprintw(win, spacePosition.y, spacePosition.x, "       ");
			if (matrix[row][col] != 0) {
				numPosition.y = 4 * row + 2;
				numPosition.x = 8 * col + 4 - mostSigDigitIndex(matrix[row][col]) / 2;
				color = pickElementColor(matrix[row][col]);
				wattron(win, COLOR_PAIR(color));
				mvwprintw(win, numPosition.y, numPosition.x,"%d", matrix[row][col]);
				wattroff(win, COLOR_PAIR(color));
			}
		}
	}
}

int mostSigDigitIndex(int num) {
	int sig_digit_index = 0;
	/* if num is 1, the output should be 0
	 * if num is 10, the output should be 1
	 * if num is 100, the output should be 2 */
	for (; num >= 10; num /= 10, sig_digit_index++);
	return sig_digit_index;
}



//this function is for choosing whether a 2 or a four will appear in an empty spot
//it returns 1 ~10% of the time and 0 ~90% of the time (the appearance of a four should be less likely than a two)
int chooseFourOrTwo(int seed) {
	srand(seed);
	int zeroOrOne = rand() % 10;
	switch (zeroOrOne) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 9:
		case 6:
		case 7:
		case 8:
			return 0;
		case 5:
			return 1;
	}
	//just in case
	return 0;
}

//this function fills a random location containing zero in the game matrix with either a 2 or a 4 (selected at random)
void fillAnEmptySquare(int m, int n, int matrix[][n]) {
	int zeroCnt = zeroCount(m, n, matrix);
	int randPlace;
	int i = 0;
	
	if (zeroCnt == 0) {
		return;
	}
	
	srand(time(NULL));
	randPlace = rand() % zeroCnt;
	
	//itterate to random spot and fill with 2 or 4
	for (int j = 0; j < m; j++) {
		for (int k = 0; k < n; k++) {
			if (matrix[j][k] == 0) {
				if (i == randPlace) {
				matrix[j][k] = 2 + 2 * chooseFourOrTwo(randPlace);					
				}
				++i;
			}
		}
	}
}

//this function is used to count the number of zeros (blank spaces) in the matrix
int zeroCount(int m, int n, int matrix[][n]) {
	int zeroCnt = 0;
	
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			if (matrix[i][j] == 0) {
				++zeroCnt;
			}
		}
	}
	
	return zeroCnt;
}

void zeroVarArray(int m, int n, int matrix[][n]) {
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			matrix[i][j] = 0;
		}
	}
}