#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h> //handle ýnput ýn real týme

#define AC_BLACK "\x1b[30m"
#define AC_RED "\x1b[31m"
#define AC_GREEN "\x1b[32m"
#define AC_WHITE "\x1b[37m"

#define SIZE 10

enum BlockTypes
{
	empty = 0,			//''
	source = 1,			//K
	target = 2,			//H
	barrier = 3,		//#
	Lpipe = 4,			//L
	Jpipe = 5,			//J
	pipe = 6,			//-
	Ppipe = 7,			//+
	verticalPipe = 8,	//|
	wall = 9			//X
};

void draw(int arr[SIZE][SIZE], int inventory[9][2]);
void handleInput(char input);
void InitializeGrid(int arr[SIZE][SIZE]);
void getrandEmptyPos(int grid[SIZE][SIZE], int randPos[2]);


int selected[2] = { 2, 2 };
int main()
{
	srand(time(NULL));

	printf(AC_WHITE);
	int grid[SIZE][SIZE];
	InitializeGrid(grid);

	int inventory[9][2] = {
		{Lpipe, 0},
		{Jpipe, 1},
		{pipe, 0},
		{Ppipe, 0},
		{verticalPipe, 2}
	};

	draw(grid, inventory);
	while (1)
	{
		char c;
		if (_kbhit()) {               // if a key is pressed
			c = _getch();		      // get the key immediately
			handleInput(c);

			fflush(stdout);
			if (c == 'q') break;    // quit
		}
		else
		{
			continue;
		}
		system("cls");
		draw(grid, inventory);
	}


	return 0;
}

void handleInput(char input) {
	switch (input)
	{
	case 'w':
		selected[0]--;
		break;
	case 'a':
		selected[1]--;
		break;
	case 's':
		selected[0]++;
		break;
	case 'd':
		selected[1]++;
		break;
	}
	if (selected[0] < 0) selected[0] = 0;
	if (selected[0] >= SIZE) selected[0] = SIZE - 1;
	if (selected[1] < 0) selected[1] = 0;
	if (selected[1] >= SIZE) selected[1] = SIZE - 1;
}

void draw(int arr[SIZE][SIZE], int inventory[9][2]) {

	//ýnventory
	printf("Inventory: \n");
	for (int i = 0; i < 9; i++)
	{
		int a = inventory[i][0];
		char* type = "";
		switch (a)
		{
		case Lpipe:
			type = "L: ";
			break;
		case Jpipe:
			type = "J: ";
			break;
		case pipe:
			type = "-: ";
			break;
		case Ppipe:
			type = "+: ";
			break;
		case verticalPipe:
			type = "|: ";
			break;
		}
		if (inventory[i][1] != 0)
			printf("%s %d pieces\n", type, inventory[i][1]);
	}
	printf("\n");


	//game
	int i, j;
	for (i = 0; i < SIZE; i++)
	{
		for (j = 0; j < SIZE; j++)
		{
			if (i == selected[0] && j == selected[1])
			{
				printf(AC_GREEN);
			}
			switch (arr[i][j])
			{
			case source:
				printf("[K]");
				break;
			case target:
				printf("[H]");
				break;
			case empty:
				printf("[ ]");
				break;
			case barrier:
				printf("[#]");
				break;
			case pipe:
				printf("[-]");
				break;
			case verticalPipe:
				printf("[|]");
				break;
			case Lpipe:
				printf("[L]");
				break;
			case Jpipe:
				printf("[J]");
				break;
			case Ppipe:
				printf("[+]");
				break;
			}
			printf(AC_WHITE);
		}
		printf("\n");
	}
}



void InitializeGrid(int arr[SIZE][SIZE])
{
	for (int i = 0; i < SIZE; i++)
		for (int j = 0; j < SIZE; j++)
			arr[i][j] = 0;

	int randPos[2] = {0,0};
	getrandEmptyPos(arr, randPos);

	arr[randPos[0]][randPos[1]] = source;

	getrandEmptyPos(arr, randPos);
	arr[randPos[0]][randPos[1]] = target;
	
	getrandEmptyPos(arr, randPos);
	arr[randPos[0]][randPos[1]] = barrier;


}
void getrandEmptyPos(int grid[SIZE][SIZE], int randPos[2]) {
	int randX = rand() % SIZE;
	int	randY = rand() % SIZE;
	if (grid[randX][randY] == empty) {
		randPos[0] = randX;
		randPos[1] = randY;
	}
	else {
		getrandEmptyPos(grid, randPos);
	}
}