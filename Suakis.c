#include<windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h> //handle input in real time

#define AC_BLACK "\x1b[30m"
#define AC_RED "\x1b[31m"
#define AC_GREEN "\x1b[32m"
#define AC_WHITE "\x1b[37m"
#define AC_YELLOW "\x1B[33m"
#define AC_BLUE "\x1B[34m"
#define RESET "\033[0m"

int GRIDSIZE = 10;

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

enum Dir
{
	UP, DOWN, LEFT, RIGHT // 0,1,2,3
};

// Structure to hold directions for each pipe type
typedef struct {
	int directions[4]; // UP, DOWN, LEFT, RIGHT (1 = connected, 0 = not connected)
} PipeDirections;

// Global array to map pipe types to their directions
PipeDirections pipeConnections[10];

/*	Prototypes	*/
void draw(int** grid, int inventory[5][2]);
void handleInput(char input);
void InitializeGrid(int** grid);
void getRandEmptyPos(int** grid, int randPos[2]);
int** createGrid(int n);
void freeGrid(int** arr, int n);
void PlacePipe();
void initializePipeConnections();
PipeDirections getPipeDirections(int pipeType);
int CheckFlow(int pos[2], int dir);
int computeFlow();
void LevelComplete();



int level = 1;

int** grid = NULL;
int** bitmap = NULL;
int sourcePos[2] = { -1, -1 };

int selectedGrid[2] = { 2, 2 };
int selectedPipe = 2;
int limitedPipes = 0; // 1: limit pipe placement to what you have in inventory 

int inventory[5][2] = {
	//   Type		 ,  amount
		{Lpipe		 , 	2},
		{Jpipe		 , 	2},
		{pipe		 , 	2},
		{Ppipe		 , 	2},
		{verticalPipe, 	2}
};

int main()
{
	srand(time(NULL));

	grid = createGrid(GRIDSIZE);
	bitmap = createGrid(GRIDSIZE);
	initializePipeConnections();
	InitializeGrid(grid);
	//TO-DO initialize inventory, based on calculating what is needed to solve the puzzle

	draw(grid, inventory);
	while (1)
	{
		char c;
		if (_kbhit()) {
			c = _getch();
			handleInput(c);
			fflush(stdout);
			if (c == 'x') { system("cls"); break; }
		}
		else { continue; }

		if (computeFlow()) {
			LevelComplete();
			continue;        // skip drawing old grid
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
		if (selectedPipe < 0) selectedPipe = 0;
		break;
	case 'e':
		selectedPipe++;
		if (selectedPipe > 4) selectedPipe = 4;
		break;

	}
	if (selectedGrid[0] < 0) selectedGrid[0] = 0;
	if (selectedGrid[0] >= GRIDSIZE) selectedGrid[0] = GRIDSIZE - 1;
	if (selectedGrid[1] < 0) selectedGrid[1] = 0;
	if (selectedGrid[1] >= GRIDSIZE) selectedGrid[1] = GRIDSIZE - 1;
}

void draw(int** grid, int inventory[5][2]) {

	system("cls");
	printf(AC_WHITE);

	//inventory
	printf("Inventory: \n");
	for (int i = 0; i < 5; i++)
	{
		if (selectedPipe == i)printf(AC_RED);

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
		}
		else {
			printf("%s\t", type);
		}
		printf(AC_WHITE);
	}
	printf("\n\n");

	//game
	int i, j;
	for (i = 0; i < GRIDSIZE; i++)
	{
		for (j = 0; j < GRIDSIZE; j++)
		{
			if (i == selectedGrid[0] && j == selectedGrid[1])
			{
				printf(AC_GREEN);
			}
			switch (grid[i][j])
			{
			case source:
				printf("\033[44;46m K \033[0m%s", RESET);
				break;
			case target:
				printf("\033[41;32m H \033[0m%s", RESET);
				break;
			case empty:
				printf("[ ]");
				break;
			case barrier:
				printf("[#]");
				break;
			case pipe:
				printf((bitmap[i][j]) ? "\033[44;46m[-]\033[0m" : "[-]");
				break;
			case verticalPipe:
				printf((bitmap[i][j]) ? "\033[44;46m[|]\033[0m" : "[|]");
				break;
			case Lpipe:
				printf((bitmap[i][j]) ? "\033[44;46m[L]\033[0m" : "[L]");
				break;
			case Jpipe:
				printf((bitmap[i][j]) ? "\033[44;46m[J]\033[0m" : "[J]");
				break;
			case Ppipe:
				printf((bitmap[i][j]) ? "\033[44;46m[+]\033[0m" : "[+]");
				break;

			}
			printf(AC_WHITE);
		}
		printf("\n");
	}

	//display controls
	printf("%s \n inventory: q/e \t square selection: WASD \t place: space \n %s", AC_YELLOW, AC_WHITE);
}

int** createGrid(int n) {
	int** arr = malloc(n * sizeof(int*));
	for (int i = 0; i < n; i++) {
		arr[i] = malloc(n * sizeof(int));
		/* zero the row so empty/bitmap start at 0 */
		for (int j = 0; j < n; j++) arr[i][j] = 0;
	}
	return arr;
}

void freeGrid(int** arr, int n) {
	for (int i = 0; i < n; i++) {
		free(arr[i]);
	}
	free(arr);
}

void generateNoise(float** noise) {
	for (int i = 0; i < GRIDSIZE; i++)
		for (int j = 0; j < GRIDSIZE; j++)
			noise[i][j] = (float)rand() / RAND_MAX;
}

float smoothNoise(int x, int y, float** noise) {
	float sum = 0;
	int count = 0;

	for (int dx = -1; dx <= 1; dx++) {
		for (int dy = -1; dy <= 1; dy++) {
			int nx = x + dx;
			int ny = y + dy;

			if (nx >= 0 && nx < GRIDSIZE && ny >= 0 && ny < GRIDSIZE) {
				sum += noise[nx][ny];
				count++;
			}
		}
	}
	return sum / count;
}

void InitializeGrid(int** grid)
{
	// generate raw noise
	float** noise = createGrid(GRIDSIZE);
	generateNoise(noise);

	//fill the grid based on noise
	for (int i = 0; i < GRIDSIZE; i++) {
		for (int j = 0; j < GRIDSIZE; j++) {
			float n = smoothNoise(i, j, noise);

			if (n < 0.35)
				grid[i][j] = barrier;      // big clusters
			else if (n < 0.38)
				grid[i][j] = verticalPipe; // rare
			else
				grid[i][j] = empty;
		}
	}

	int randPos[2] = { 0,0 };
	getRandEmptyPos(grid, randPos);

	grid[randPos[0]][randPos[1]] = source;
	sourcePos[0] = randPos[0];
	sourcePos[1] = randPos[1];

	getRandEmptyPos(grid, randPos);
	grid[randPos[0]][randPos[1]] = target;

}

void PlacePipe() {
	if (grid[selectedGrid[0]][selectedGrid[1]] != 0) return; // only place to empty

	if (limitedPipes == 0)
	{
		grid[selectedGrid[0]][selectedGrid[1]] = inventory[selectedPipe][0];
		return;
	}

	if (inventory[selectedPipe][1] < 1) return; // no pipes to place

	grid[selectedGrid[0]][selectedGrid[1]] = inventory[selectedPipe][0]; //place
	inventory[selectedPipe][1]--; //reduce inventory amount
}

void getRandEmptyPos(int** grid, int randPos[2]) {
	int randX = rand() % GRIDSIZE;
	int	randY = rand() % GRIDSIZE;
	if (grid[randX][randY] == empty) {
		randPos[0] = randX;
		randPos[1] = randY;
	}
	else {
		getRandEmptyPos(grid, randPos);
	}
}

void initializePipeConnections() {
	// Initialize all to zeros
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 4; j++) {
			pipeConnections[i].directions[j] = 0;
		}
	}

	// Horizontal pipe: - (connects LEFT and RIGHT)
	pipeConnections[pipe].directions[LEFT] = 1;
	pipeConnections[pipe].directions[RIGHT] = 1;

	// Vertical pipe: | (connects UP and DOWN)
	pipeConnections[verticalPipe].directions[UP] = 1;
	pipeConnections[verticalPipe].directions[DOWN] = 1;

	// L pipe: L (connects UP and RIGHT)
	pipeConnections[Lpipe].directions[UP] = 1;
	pipeConnections[Lpipe].directions[RIGHT] = 1;

	// J pipe: J (connects UP and LEFT)
	pipeConnections[Jpipe].directions[UP] = 1;
	pipeConnections[Jpipe].directions[LEFT] = 1;

	// Plus pipe: + (connects all four directions)
	pipeConnections[Ppipe].directions[UP] = 1;
	pipeConnections[Ppipe].directions[DOWN] = 1;
	pipeConnections[Ppipe].directions[LEFT] = 1;
	pipeConnections[Ppipe].directions[RIGHT] = 1;
}

PipeDirections getPipeDirections(int pipeType) {
	return pipeConnections[pipeType];
}

int oppositeDir(int d) {
	switch (d) {
	case UP:    return DOWN;
	case DOWN:  return UP;
	case LEFT:  return RIGHT;
	case RIGHT: return LEFT;
	default:    return -1;
	}
}

int CheckFlow(int pos[2], int dir) {
	/* pos may be e.g. sourcePosArr */
	if (!pos) return 0;

	int x = pos[0], y = pos[1];

	/* bounds check */
	if (x < 0 || x >= GRIDSIZE || y < 0 || y >= GRIDSIZE) return 0;

	/* if already visited, stop */
	if (bitmap[x][y]) return 0;

	/* mark visited */
	bitmap[x][y] = 1;

	/* if we hit target: you win; could set a flag here */
	if (grid[x][y] == target) {
		return 1;
	}

	/* handle the source specially: try all 4 neighbors */
	if (grid[x][y] == source) {
		for (int d = 0; d < 4; ++d) {
			int nx = x, ny = y;
			if (d == UP)    nx = x - 1;
			if (d == DOWN)  nx = x + 1;
			if (d == LEFT)  ny = y - 1;
			if (d == RIGHT) ny = y + 1;

			if (nx < 0 || nx >= GRIDSIZE || ny < 0 || ny >= GRIDSIZE) continue;
			if (bitmap[nx][ny]) continue;
			int ntype = grid[nx][ny];
			if (ntype == empty || ntype == barrier) continue;

			/* check reciprocal connection of neighbor */
			PipeDirections ndirs = getPipeDirections(ntype);
			if (ndirs.directions[oppositeDir(d)]) {
				int npos[2] = { nx, ny };
				if (CheckFlow(npos, oppositeDir(d)))   // ★ NEW
					return 1;                          // ★ NEW propagate
			}
			else if (ntype == target) {
				int npos[2] = { nx, ny };
				if (CheckFlow(npos, oppositeDir(d))) return 1;
			}
		}
		return 0;
	}

	/* Normal pipe (not source): check its outgoing directions */
	PipeDirections dirs = getPipeDirections(grid[x][y]);
	for (int d = 0; d < 4; ++d) {
		if (!dirs.directions[d]) continue;   /* pipe doesn't face this way */
		if (dir != -1 && d == dir) continue; /* don't go back the way we came */

		int nx = x, ny = y;
		if (d == UP)    nx = x - 1;
		if (d == DOWN)  nx = x + 1;
		if (d == LEFT)  ny = y - 1;
		if (d == RIGHT) ny = y + 1;

		if (nx < 0 || nx >= GRIDSIZE || ny < 0 || ny >= GRIDSIZE) continue;
		if (bitmap[nx][ny]) continue;

		int ntype = grid[nx][ny];
		if (ntype == empty || ntype == barrier) continue;

		/* neighbor must have a reciprocal connection (or be target) */
		PipeDirections ndirs = getPipeDirections(ntype);
		int opp = oppositeDir(d);
		if (ntype == target || ndirs.directions[opp]) {
			int npos[2] = { nx, ny };
			if (CheckFlow(npos, opp)) return 1;
		}
	}
	return 0;
}

int computeFlow() {
	/* clear bitmap */
	for (int i = 0; i < GRIDSIZE; i++)
		for (int j = 0; j < GRIDSIZE; j++)
			bitmap[i][j] = 0;

	/* ensure sourcePos is valid */
	if (sourcePos[0] >= 0)
		return CheckFlow(sourcePos, -1);
}

void LevelComplete() {
	draw(grid, inventory);
	Sleep(500);
	system("cls");
	printf("Level %d completed!\n", level);
	Sleep(1000);
	level++;
	if (level == 3) limitedPipes = 1; //enable limited pipes from level 3
	GRIDSIZE += 5;
	grid = createGrid(GRIDSIZE);
	bitmap = createGrid(GRIDSIZE);
	InitializeGrid(grid);
	draw(grid, inventory);
}  