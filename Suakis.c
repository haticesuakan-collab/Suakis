// cleaned_pipes_game.c
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>

#define AC_WHITE "\x1b[37m"
#define AC_RED   "\x1b[31m"
#define AC_GREEN "\x1b[32m"
#define AC_YELLOW "\x1B[33m"
#define RESET    "\033[0m"

int GRIDSIZE = 10;
int level = 1;

/* -------------------------
   Types & Globals
   ------------------------- */
typedef struct { int x, y; } Node;

int **grid = NULL;
int **bitmap = NULL;
int sourcePos[2] = { -1, -1 };
int targetPos[2] = { -1, -1 };

typedef struct {
    int x, y;
    int g, h, f;
    int parentX, parentY;
    int opened, closed;
} AStarNode;

/* block types */
enum BlockTypes {
    empty = 0,
    source = 1,
    target = 2,
    barrier = 3,
    Lpipe = 4,
    Jpipe = 5,
    pipe = 6,        // -
    Ppipe = 7,       // +
    verticalPipe = 8,// |
    wall = 9
};

/* inventory: pair [type, amount] */
int inventory[5][2] = {
    {Lpipe, 0},
    {Jpipe, 0},
    {pipe, 0},
    {Ppipe, 0},
    {verticalPipe, 0}
};

/* pipe connection table (UP, DOWN, LEFT, RIGHT) */
typedef struct { int directions[4]; } PipeDirections;
PipeDirections pipeConnections[10];

enum Dir { UP, DOWN, LEFT, RIGHT };

/* selection & UI */
int selectedGrid[2] = { 2, 2 };
int selectedPipe = 2;
int limitedPipes = 1; // if 1, placing consumes inventory

/* -------------------------
   Prototypes
   ------------------------- */
int **createGrid(int n);
void freeGrid(int **arr, int n);
void initializePipeConnections(void);
Node* aStarPath(int *outLength);
void initializeInventory(void);
int pipeForConnection(int dx1, int dy1, int dx2, int dy2);

void InitializeGrid(int **grid);
void getRandEmptyPos(int **grid, int randPos[2]);
void initializeNoiseGrid(int **grid);

void draw(int **grid, int inventoryArr[5][2]);
void handleInput(char input);
void PlacePipe(void);

int CheckFlow(int pos[2], int dir);
int computeFlow(void);
void LevelComplete(void);

/* -------------------------
   Utility: create/free grid
   ------------------------- */
int **createGrid(int n) {
    int **arr = malloc(n * sizeof(int*));
    if (!arr) { perror("malloc"); exit(1); }
    for (int i = 0; i < n; ++i) {
        arr[i] = malloc(n * sizeof(int));
        if (!arr[i]) { perror("malloc"); exit(1); }
        for (int j = 0; j < n; ++j) arr[i][j] = empty;
    }
    return arr;
}

void freeGrid(int **arr, int n) {
    if (!arr) return;
    for (int i = 0; i < n; ++i) free(arr[i]);
    free(arr);
}

/* -------------------------
   Pipe connections table
   ------------------------- */
void initializePipeConnections(void) {
    for (int i = 0; i < 10; ++i)
        for (int j = 0; j < 4; ++j)
            pipeConnections[i].directions[j] = 0;

    /* horizontal - */
    pipeConnections[pipe].directions[LEFT] = 1;
    pipeConnections[pipe].directions[RIGHT] = 1;

    /* vertical | */
    pipeConnections[verticalPipe].directions[UP] = 1;
    pipeConnections[verticalPipe].directions[DOWN] = 1;

    /* L (up + right) */
    pipeConnections[Lpipe].directions[UP] = 1;
    pipeConnections[Lpipe].directions[RIGHT] = 1;

    /* J (down + left) */
    pipeConnections[Jpipe].directions[DOWN] = 1;
    pipeConnections[Jpipe].directions[LEFT] = 1;

    /* + */
    pipeConnections[Ppipe].directions[UP] = 1;
    pipeConnections[Ppipe].directions[DOWN] = 1;
    pipeConnections[Ppipe].directions[LEFT] = 1;
    pipeConnections[Ppipe].directions[RIGHT] = 1;
}

PipeDirections getPipeDirections(int pipeType) {
    return pipeConnections[pipeType];
}

/* -------------------------
   Random noise-based initialization
   ------------------------- */
void generateNoise(float noise[GRIDSIZE][GRIDSIZE]) {
    for (int i = 0; i < GRIDSIZE; ++i)
        for (int j = 0; j < GRIDSIZE; ++j)
            noise[i][j] = (float)rand() / RAND_MAX;
}

float smoothNoise(int x, int y, float noise[GRIDSIZE][GRIDSIZE]) {
    float sum = 0; int count = 0;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < GRIDSIZE && ny >= 0 && ny < GRIDSIZE) {
                sum += noise[nx][ny];
                ++count;
            }
        }
    return (count>0) ? (sum / count) : 0.0f;
}

/* place source and target and fill noise tiles */
void InitializeGrid(int **grid) {
    initializeNoiseGrid(grid);

    int randPos[2] = {0,0};
    getRandEmptyPos(grid, randPos);
    grid[randPos[0]][randPos[1]] = source;
    sourcePos[0] = randPos[0]; sourcePos[1] = randPos[1];

    // ensure target is not placed adjacent (manhattan distance >= 3)
    int tries = GRIDSIZE * GRIDSIZE * 4;
    do {
        getRandEmptyPos(grid, randPos);
        tries--;
        if (tries <= 0) break; // fallback if grid very full
    } while (abs(randPos[0] - sourcePos[0]) + abs(randPos[1] - sourcePos[1]) < 3);

    grid[randPos[0]][randPos[1]] = target;
    targetPos[0] = randPos[0]; targetPos[1] = randPos[1];
}

/* Fill grid based on smoothed random noise */
void initializeNoiseGrid(int **grid) {
    float noise[GRIDSIZE][GRIDSIZE];
    generateNoise(noise);
    for (int i = 0; i < GRIDSIZE; ++i)
        for (int j = 0; j < GRIDSIZE; ++j) {
            float n = smoothNoise(i, j, noise);
            if (n < 0.35f) grid[i][j] = barrier;
            else if (n < 0.38f) grid[i][j] = verticalPipe;
            else grid[i][j] = empty;
        }
}

/* safer iterative random empty finder */
void getRandEmptyPos(int **grid, int randPos[2]) {
    int tries = GRIDSIZE * GRIDSIZE * 2;
    while (tries--) {
        int rx = rand() % GRIDSIZE;
        int ry = rand() % GRIDSIZE;
        if (grid[rx][ry] == empty) { randPos[0] = rx; randPos[1] = ry; return; }
    }
    /* fallback linear scan */
    for (int i = 0; i < GRIDSIZE; ++i)
        for (int j = 0; j < GRIDSIZE; ++j)
            if (grid[i][j] == empty) { randPos[0] = i; randPos[1] = j; return; }

    randPos[0] = randPos[1] = -1;
}

/* -------------------------
   A* pathfinding (returns static array pointer)
   ------------------------- */
int isWalkable(int type) { return type != barrier; }

Node* aStarPath(int* outLength) {
    static AStarNode nodes[100][100];
    static Node pathBuf[10000];
    *outLength = 0;

    if (sourcePos[0] < 0 || targetPos[0] < 0) return NULL;
    int sx = sourcePos[0], sy = sourcePos[1], tx = targetPos[0], ty = targetPos[1];

    if (GRIDSIZE > 100) { printf("Grid too large for A* static buffer\n"); return NULL; }

    for (int i = 0; i < GRIDSIZE; ++i)
        for (int j = 0; j < GRIDSIZE; ++j) {
            nodes[i][j].x = i; nodes[i][j].y = j;
            nodes[i][j].g = nodes[i][j].h = nodes[i][j].f = 999999;
            nodes[i][j].opened = nodes[i][j].closed = 0;
        }

    AStarNode* start = &nodes[sx][sy];
    start->g = 0;
    start->h = abs(sx - tx) + abs(sy - ty);
    start->f = start->g + start->h;
    start->opened = 1;

    while (1) {
        AStarNode* current = NULL;
        for (int i = 0; i < GRIDSIZE; ++i)
            for (int j = 0; j < GRIDSIZE; ++j)
                if (nodes[i][j].opened && !nodes[i][j].closed)
                    if (!current || nodes[i][j].f < current->f)
                        current = &nodes[i][j];

        if (!current) return NULL; // no path

        if (current->x == tx && current->y == ty) {
            int length = 0;
            AStarNode* cur = current;
            while (!(cur->x == sx && cur->y == sy)) {
                pathBuf[length].x = cur->x;
                pathBuf[length].y = cur->y;
                ++length;
                int px = cur->parentX, py = cur->parentY;
                cur = &nodes[px][py];
            }
            pathBuf[length].x = sx; pathBuf[length].y = sy; ++length;
            /* reverse */
            for (int a = 0; a < length/2; ++a) {
                Node tmp = pathBuf[a];
                pathBuf[a] = pathBuf[length - a - 1];
                pathBuf[length - a - 1] = tmp;
            }
            *outLength = length;
            return pathBuf;
        }

        current->opened = 0;
        current->closed = 1;

        int dirs[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
        for (int d = 0; d < 4; ++d) {
            int nx = current->x + dirs[d][0];
            int ny = current->y + dirs[d][1];
            if (nx < 0 || nx >= GRIDSIZE || ny < 0 || ny >= GRIDSIZE) continue;
            if (!isWalkable(grid[nx][ny]) && !(nx==tx && ny==ty)) continue;

            AStarNode* nb = &nodes[nx][ny];
            if (nb->closed) continue;

            int gNew = current->g + 1;
            if (!nb->opened || gNew < nb->g) {
                nb->g = gNew;
                nb->h = abs(nx - tx) + abs(ny - ty);
                nb->f = nb->g + nb->h;
                nb->parentX = current->x;
                nb->parentY = current->y;
                nb->opened = 1;
            }
        }
    }
    return NULL;
}

/* -------------------------
   Convert path into minimal inventory
   ------------------------- */
// dx/dy encoding:
// (-1,0) = UP, (1,0) = DOWN, (0,-1) = LEFT, (0,1) = RIGHT
int dirOf(int dx, int dy) {
    if (dx == -1 && dy == 0) return UP;
    if (dx == 1  && dy == 0) return DOWN;
    if (dx == 0  && dy == -1) return LEFT;
    if (dx == 0  && dy == 1) return RIGHT;
    return -1; // error
}
dd
int pipeForConnection(int dx1, int dy1, int dx2, int dy2)
{
    int d1 = dirOf(dx1, dy1);
    int d2 = dirOf(dx2, dy2);

    // straight pipes
    if ((d1 == LEFT && d2 == RIGHT) || (d1 == RIGHT && d2 == LEFT))
        return pipe;
    if ((d1 == UP && d2 == DOWN) || (d1 == DOWN && d2 == UP))
        return verticalPipe;

    // only EXACT corner shapes allowed:
    // Lpipe: UP + RIGHT
    if ((d1 == UP && d2 == RIGHT) || (d1 == RIGHT && d2 == UP))
        return Lpipe;

    // Jpipe: UP + LEFT
    if ((d1 == UP && d2 == LEFT) || (d1 == LEFT && d2 == UP))
        return Jpipe;

    // everything else → plus pipe
    return Ppipe;
}

void initializeInventory(void) {
    for (int i = 0; i < 5; ++i) inventory[i][1] = 0;
    int length = 0;
    Node *path = aStarPath(&length);
    if (!path) { printf("ERROR: No path exists (inventory init)\n"); exit(1); }
    /* path includes source and target; pipes are placed at intermediate cells */
    for (int i = 1; i < length - 1; ++i) {
        int xPrev = path[i-1].x, yPrev = path[i-1].y;
        int xCurr = path[i].x,  yCurr = path[i].y;
        int xNext = path[i+1].x, yNext = path[i+1].y;
        int dx1 = xCurr - xPrev, dy1 = yCurr - yPrev;
        int dx2 = xNext - xCurr, dy2 = yNext - yCurr;
        int t = pipeForConnection(dx1, dy1, dx2, dy2);
        for (int k = 0; k < 5; ++k) if (inventory[k][0] == t) inventory[k][1]++;
    }
}

/* -------------------------
   Flow check (recursively, safe)
   ------------------------- */
int oppositeDir(int d) {
    switch (d) {
        case UP: return DOWN;
        case DOWN: return UP;
        case LEFT: return RIGHT;
        case RIGHT: return LEFT;
    }
    return -1;
}

int CheckFlow(int pos[2], int dir) {
    if (!pos) return 0;
    int x = pos[0], y = pos[1];
    if (x < 0 || x >= GRIDSIZE || y < 0 || y >= GRIDSIZE) return 0;
    if (bitmap[x][y]) return 0;
    bitmap[x][y] = 1;

    if (grid[x][y] == target) return 1;

    /* If source: try all neighbors */
    if (grid[x][y] == source) {
        for (int d = 0; d < 4; ++d) {
            int nx = x + (d==UP?-1:d==DOWN?1:0);
            int ny = y + (d==LEFT?-1:d==RIGHT?1:0);
            if (nx < 0 || nx >= GRIDSIZE || ny < 0 || ny >= GRIDSIZE) continue;
            if (bitmap[nx][ny]) continue;
            int ntype = grid[nx][ny];
            if (ntype == empty || ntype == barrier) continue;
            if (ntype == target) { int npos[2] = {nx,ny}; if (CheckFlow(npos, oppositeDir(d))) return 1; }
            PipeDirections nd = getPipeDirections(ntype);
            int opp = oppositeDir(d);
            if (opp >= 0 && nd.directions[opp]) { int npos[2] = {nx,ny}; if (CheckFlow(npos, opp)) return 1; }
        }
        return 0;
    }

    /* Normal pipe: follow its connected directions */
    PipeDirections pd = getPipeDirections(grid[x][y]);
    for (int d = 0; d < 4; ++d) {
        if (!pd.directions[d]) continue;
        if (dir != -1 && d == dir) continue; // do not go back where we came from
        int nx = x + (d==UP?-1:d==DOWN?1:0);
        int ny = y + (d==LEFT?-1:d==RIGHT?1:0);
        if (nx < 0 || nx >= GRIDSIZE || ny < 0 || ny >= GRIDSIZE) continue;
        if (bitmap[nx][ny]) continue;
        int ntype = grid[nx][ny];
        if (ntype == empty || ntype == barrier) continue;
        if (ntype == target) { int npos[2]={nx,ny}; if (CheckFlow(npos, oppositeDir(d))) return 1; }
        PipeDirections nd = getPipeDirections(ntype);
        int opp = oppositeDir(d);
        if (opp >= 0 && nd.directions[opp]) { int npos[2] = {nx,ny}; if (CheckFlow(npos, opp)) return 1; }
    }
    return 0;
}

int computeFlow(void) {
    for (int i = 0; i < GRIDSIZE; ++i)
        for (int j = 0; j < GRIDSIZE; ++j)
            bitmap[i][j] = 0;
    if (sourcePos[0] >= 0) return CheckFlow(sourcePos, -1);
    return 0;
}

/* -------------------------
   Drawing & input
   ------------------------- */
void draw(int **grid, int inventoryArr[5][2]) {
    // (Do not clear screen before calling computeFlow in main; here just draw)
    printf(AC_WHITE);
    printf("Level %d | Grid %dx%d\n", level, GRIDSIZE, GRIDSIZE);

    printf("Inventory: ");
    for (int i = 0; i < 5; ++i) {
        if (selectedPipe == i) printf(AC_RED);
        int a = inventoryArr[i][0];
        char *s = a==Lpipe?"L": a==Jpipe?"J": a==pipe?"-": a==Ppipe?"+":"|";
        if (limitedPipes) printf("%s:%d\t", s, inventoryArr[i][1]); else printf("%s\t", s);
        printf(AC_WHITE);
    }
    printf("\n\n");

    for (int i = 0; i < GRIDSIZE; ++i) {
        for (int j = 0; j < GRIDSIZE; ++j) {
            if (i == selectedGrid[0] && j == selectedGrid[1]) printf(AC_GREEN);
            switch (grid[i][j]) {
                case source:       printf("\033[44;46m K \033[0m%s", RESET); break;
                case target:       printf("\033[41;32m H \033[0m%s", RESET); break;
                case empty:        printf("[ ]"); break;
                case barrier:      printf("[#]"); break;
                case pipe:         printf((bitmap[i][j])?"\033[44;46m[-]\033[0m":"[-]"); break;
                case verticalPipe: printf((bitmap[i][j])?"\033[44;46m[|]\033[0m":"[|]"); break;
                case Lpipe:        printf((bitmap[i][j])?"\033[44;46m[L]\033[0m":"[L]"); break;
                case Jpipe:        printf((bitmap[i][j])?"\033[44;46m[J]\033[0m":"[J]"); break;
                case Ppipe:        printf((bitmap[i][j])?"\033[44;46m[+]\033[0m":"[+]"); break;
                default:           printf("[?]"); break;
            }
            printf(AC_WHITE);
        }
        printf("\n");
    }
    printf("%s\n inventory: q/e \t square selection: WASD \t place: space \n %s", AC_YELLOW, AC_WHITE);
}

void handleInput(char input) {
    switch (input) {
        case 'w': selectedGrid[0]--; break;
        case 'a': selectedGrid[1]--; break;
        case 's': selectedGrid[0]++; break;
        case 'd': selectedGrid[1]++; break;
        case ' ': PlacePipe(); break;
        case 'q': selectedPipe--; if (selectedPipe < 0) selectedPipe = 0; break;
        case 'e': selectedPipe++; if (selectedPipe > 4) selectedPipe = 4; break;
    }
    if (selectedGrid[0] < 0) selectedGrid[0] = 0;
    if (selectedGrid[0] >= GRIDSIZE) selectedGrid[0] = GRIDSIZE - 1;
    if (selectedGrid[1] < 0) selectedGrid[1] = 0;
    if (selectedGrid[1] >= GRIDSIZE) selectedGrid[1] = GRIDSIZE - 1;
}

void PlacePipe(void) {
    int x = selectedGrid[0], y = selectedGrid[1];
    if (grid[x][y] != empty) return;
    int t = inventory[selectedPipe][0];
    if (limitedPipes) {
        if (inventory[selectedPipe][1] < 1) return;
        inventory[selectedPipe][1]--;
    }
    grid[x][y] = t;
}

/* -------------------------
   Level completion and lifecycle
   ------------------------- */
void LevelComplete(void) {
    draw(grid, inventory);
    Sleep(400);
    system("cls");
    printf("Level %d complete!\n", level);
    Sleep(800);

    /* free old */
    freeGrid(grid, GRIDSIZE);
    freeGrid(bitmap, GRIDSIZE);

    level++;
    GRIDSIZE = 10 + level; /* example growth rule */
    grid = createGrid(GRIDSIZE);
    bitmap = createGrid(GRIDSIZE);
    initializePipeConnections();
    InitializeGrid(grid);
    initializeInventory();
}

/* -------------------------
   Main
   ------------------------- */
int main(void) {
    srand((unsigned)time(NULL));

    grid = createGrid(GRIDSIZE);
    bitmap = createGrid(GRIDSIZE);
    initializePipeConnections();
    InitializeGrid(grid);

    /* compute minimal inventory from shortest path */
    initializeInventory();

    /* initial draw */
    system("cls");
    draw(grid, inventory);

    /* main loop */
    while (1) {
        if (_kbhit()) {
            char c = _getch();
            if (c == 'x') { system("cls"); break; }
            handleInput(c);
        }else{continue;}

        /* compute flow every loop */
        if (computeFlow()) {
            LevelComplete();
            system("cls");
            draw(grid, inventory);
            continue;
        }

        system("cls");
        draw(grid, inventory);

        Sleep(60); /* avoid 100% CPU busy loop */
    }

    /* cleanup */
    freeGrid(grid, GRIDSIZE);
    freeGrid(bitmap, GRIDSIZE);

    return 0;
}
