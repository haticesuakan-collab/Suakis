#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h> //handle input in real time

#define AC_BLACK "\x1b[30m"
#define AC_RED "\x1b[31m"
#define AC_GREEN "\x1b[32m"
#define AC_WHITE "\x1b[37m"

int SIZE = 10;

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

void draw(int **grid, int inventory[5][2]);
void handleInput(char input);
void InitializeGrid(int **grid);
void getRandEmptyPos(int **grid, int randPos[2]);
int **createGrid(int n);
void freeGrid(int **arr, int n);
void PlacePipe();

int **grid = NULL;

int selectedGrid[2] = { 2, 2 };
int selectedPipe = 2;
int limitedPipes = 1; // 1: limit pipe placement to what you have in inventory 

int inventory[5][2] = {
	{Lpipe, 2},
	{Jpipe, 1},
	{pipe, 1},
	{Ppipe, 1},
	{verticalPipe, 1}
};

int main()
{
	srand(time(NULL));

	printf(AC_WHITE);
	grid = createGrid(SIZE);
	InitializeGrid(grid);


	draw(grid, inventory);
	while (1)
	{
		char c;
		if (_kbhit()) {               // if a key is pressed
			c = _getch();		      // get the key immediately
			handleInput(c);

			fflush(stdout);
			if (c == 'x') break;    // quit
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
		selectedGrid[0]--;
		break;
	case 'a':
		selectedGrid[1]--;
		break;
	case 's':
		selectedGrid[0]++;
		break;
	case 'd':
		selectedGrid[1]++;
		break;
	case ' ':
		PlacePipe();
		break;

	//inventory selection
	case 'q':
		selectedPipe--;
		if (selectedPipe < 0) selectedPipe=0;
		break;
	case 'e':
		selectedPipe++;
		if (selectedPipe > 4) selectedPipe=4;
		break;
	
	}
	if (selectedGrid[0] < 0) selectedGrid[0] = 0;
	if (selectedGrid[0] >= SIZE) selectedGrid[0] = SIZE - 1;
	if (selectedGrid[1] < 0) selectedGrid[1] = 0;
	if (selectedGrid[1] >= SIZE) selectedGrid[1] = SIZE - 1;
}

void draw(int **grid, int inventory[5][2]) {

	//inventory
	printf("Inventory: \n");
	for (int i = 0; i < 5; i++)
	{
		if(selectedPipe == i)printf(AC_RED);

		int a = inventory[i][0];
		char* type = "";
		switch (a)
		{
		case Lpipe:
			type = "L";
			break;
		case Jpipe:
			type = "J";
			break;
		case pipe:
			type = "-";
			break;
		case Ppipe:
			type = "+";
			break;
		case verticalPipe:
			type = "|";
			break;
		}
		if (limitedPipes == 1) {
			printf("%s :  %d pieces\t", type, inventory[i][1]);
		}else{
			printf("%s\t", type);
		}
		printf(AC_WHITE);
	}
	printf("\n");


	//game
	int i, j;
	for (i = 0; i < SIZE; i++)
	{
		for (j = 0; j < SIZE; j++)
		{
			if (i == selectedGrid[0] && j == selectedGrid[1])
			{
				printf(AC_GREEN);
			}
			switch (grid[i][j])
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
			default:
				printf("Err!");
				break;
			}
			printf(AC_WHITE);
		}
		printf("\n");
	}

	//display controls
	printf("\n inventory: q/e \t square selection: WASD \t place: space \n");
}

int **createGrid(int n) {
    int **arr = malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        arr[i] = malloc(n * sizeof(int));
    }
    return arr;
}

void freeGrid(int **arr, int n) {
    for (int i = 0; i < n; i++) {
        free(arr[i]);
    }
    free(arr);
}


void InitializeGrid(int **grid)
{
	for (int i = 0; i < SIZE; i++)
		for (int j = 0; j < SIZE; j++)
			grid[i][j] = 0;

	int randPos[2] = {0,0};
	getRandEmptyPos(grid, randPos);

	grid[randPos[0]][randPos[1]] = source;

	getRandEmptyPos(grid, randPos);
	grid[randPos[0]][randPos[1]] = target;
	
	getRandEmptyPos(grid, randPos);
	grid[randPos[0]][randPos[1]] = barrier;
}

void PlacePipe(){
	if(grid[selectedGrid[0]][selectedGrid[1]] != 0) return; // only place to empty

	if (limitedPipes == 0)
	{
		grid[selectedGrid[0]][selectedGrid[1]] = inventory[selectedPipe][0];
		return;
	}
	
	if (inventory[selectedPipe][1] < 1) return; // no pipes to place

	grid[selectedGrid[0]][selectedGrid[1]] = inventory[selectedPipe][0]; //place
	inventory[selectedPipe][1]--; //reduce inventory amount
}

void getRandEmptyPos(int **grid, int randPos[2]) {
	int randX = rand() % SIZE;
	int	randY = rand() % SIZE;
	if (grid[randX][randY] == empty) {
		randPos[0] = randX;
		randPos[1] = randY;
	}
	else {
		getRandEmptyPos(grid, randPos);
	}
}