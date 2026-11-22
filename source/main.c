// Snake DS - petit jeu inspire de snake.io (code original)
// Ecran du haut : le jeu. Ecran du bas : score et infos.

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GW 32            // largeur de la grille (cases)
#define GH 24            // hauteur de la grille (cases)
#define CELL 8           // taille d'une case en pixels
#define MAXLEN 300
#define NSNAKES 4        // 1 joueur + 3 serpents IA
#define MAXFOOD 40
#define MINFOOD 20
#define FRAMES_PER_MOVE 7

typedef struct {
    int x[MAXLEN], y[MAXLEN];
    int len, dx, dy, alive;
    u16 color;
} Snake;

static Snake snakes[NSNAKES];
static int foodx[MAXFOOD], foody[MAXFOOD], foodon[MAXFOOD];
static int respawn[NSNAKES];
static int nextdx = 1, nextdy = 0;
static int gameover = 0, best = 0;

static int occupied(int x, int y) {
    int i, k;
    for (i = 0; i < NSNAKES; i++) {
        if (!snakes[i].alive) continue;
        for (k = 0; k < snakes[i].len; k++)
            if (snakes[i].x[k] == x && snakes[i].y[k] == y) return 1;
    }
    return 0;
}

static int foodDist(int x, int y) {
    int f, d, b = 999;
    for (f = 0; f < MAXFOOD; f++) {
        if (!foodon[f]) continue;
        d = abs(foodx[f] - x) + abs(foody[f] - y);
        if (d < b) b = d;
    }
    return (b == 999) ? 0 : b;
}

static void placeFood(int f) {
    int tries = 0, x, y;
    do {
        x = rand() % GW;
        y = rand() % GH;
        tries++;
    } while (tries < 60 && occupied(x, y));
    foodx[f] = x;
    foody[f] = y;
    foodon[f] = 1;
}

static void dropFood(Snake *s) {
    int k, f = 0;
    for (k = 0; k < s->len; k += 2) {
        while (f < MAXFOOD && foodon[f]) f++;
        if (f >= MAXFOOD) break;
        foodx[f] = s->x[k];
        foody[f] = s->y[k];
        foodon[f] = 1;
    }
}

static void spawnSnake(int i) {
    Snake *s = &snakes[i];
    int tries = 0, x, y, k, ok;
    s->alive = 0;
    do {
        x = 4 + rand() % (GW - 8);
        y = 3 + rand() % (GH - 6);
        ok = 1;
        for (k = -2; k <= 3; k++)
            if (occupied(x + k, y)) ok = 0;
        tries++;
    } while (!ok && tries < 100);
    s->len = 3;
    for (k = 0; k < 3; k++) {
        s->x[k] = x - k;
        s->y[k] = y;
    }
    s->dx = 1;
    s->dy = 0;
    s->alive = 1;
}

static void aiSteer(int i) {
    Snake *s = &snakes[i];
    int dirx[3], diry[3];
    int c, bestc = 0, bestcost = 100000, nx, ny, cost;
    dirx[0] = s->dx;   diry[0] = s->dy;     // tout droit
    dirx[1] = s->dy;   diry[1] = -s->dx;    // un cote
    dirx[2] = -s->dy;  diry[2] = s->dx;     // l'autre cote
    for (c = 0; c < 3; c++) {
        nx = s->x[0] + dirx[c];
        ny = s->y[0] + diry[c];
        if (nx < 0 || ny < 0 || nx >= GW || ny >= GH || occupied(nx, ny))
            cost = 5000;
        else
            cost = foodDist(nx, ny) * 2 + rand() % 5;
        if (cost < bestcost) {
            bestcost = cost;
            bestc = c;
        }
    }
    s->dx = dirx[bestc];
    s->dy = diry[bestc];
}

static void tick(void) {
    int nx[NSNAKES], ny[NSNAKES], dead[NSNAKES];
    int i, j, k, f, ate, tx, ty, count;
    Snake *s;

    snakes[0].dx = nextdx;
    snakes[0].dy = nextdy;
    for (i = 1; i < NSNAKES; i++)
        if (snakes[i].alive) aiSteer(i);

    for (i = 0; i < NSNAKES; i++) {
        s = &snakes[i];
        dead[i] = 0;
        nx[i] = 0;
        ny[i] = 0;
        if (!s->alive) continue;
        nx[i] = s->x[0] + s->dx;
        ny[i] = s->y[0] + s->dy;
        if (nx[i] < 0 || ny[i] < 0 || nx[i] >= GW || ny[i] >= GH ||
            occupied(nx[i], ny[i]))
            dead[i] = 1;
    }
    // collision tete contre tete
    for (i = 0; i < NSNAKES; i++)
        for (j = i + 1; j < NSNAKES; j++)
            if (snakes[i].alive && snakes[j].alive &&
                nx[i] == nx[j] && ny[i] == ny[j]) {
                dead[i] = 1;
                dead[j] = 1;
            }

    for (i = 0; i < NSNAKES; i++) {
        s = &snakes[i];
        if (!s->alive) {
            if (i > 0 && --respawn[i] <= 0) spawnSnake(i);
            continue;
        }
        if (dead[i]) {
            dropFood(s);
            s->alive = 0;
            respawn[i] = 25;
            if (i == 0) gameover = 1;
            continue;
        }
        tx = s->x[s->len - 1];
        ty = s->y[s->len - 1];
        for (k = s->len - 1; k > 0; k--) {
            s->x[k] = s->x[k - 1];
            s->y[k] = s->y[k - 1];
        }
        s->x[0] = nx[i];
        s->y[0] = ny[i];
        ate = 0;
        for (f = 0; f < MAXFOOD; f++)
            if (foodon[f] && foodx[f] == nx[i] && foody[f] == ny[i]) {
                foodon[f] = 0;
                ate++;
            }
        while (ate > 0 && s->len < MAXLEN) {
            s->x[s->len] = tx;
            s->y[s->len] = ty;
            s->len++;
            ate--;
        }
    }

    count = 0;
    for (f = 0; f < MAXFOOD; f++)
        if (foodon[f]) count++;
    for (f = 0; f < MAXFOOD && count < MINFOOD; f++)
        if (!foodon[f]) {
            placeFood(f);
            count++;
        }

    if (snakes[0].len - 3 > best) best = snakes[0].len - 3;
}

static void fillCell(int cx, int cy, u16 c) {
    u16 *fb = VRAM_A;
    int px = cx * CELL, py = cy * CELL, x, y;
    for (y = py; y < py + CELL - 1; y++)
        for (x = px; x < px + CELL - 1; x++)
            fb[y * 256 + x] = c;
}

static void render(void) {
    u16 *fb = VRAM_A;
    int i, k, f;
    u16 bg = ARGB16(1, 2, 2, 5);
    for (i = 0; i < 256 * 192; i++) fb[i] = bg;
    for (f = 0; f < MAXFOOD; f++)
        if (foodon[f]) fillCell(foodx[f], foody[f], ARGB16(1, 31, 20, 4));
    for (i = 0; i < NSNAKES; i++) {
        if (!snakes[i].alive) continue;
        for (k = snakes[i].len - 1; k >= 0; k--)
            fillCell(snakes[i].x[k], snakes[i].y[k], snakes[i].color);
        if (i == 0)
            fillCell(snakes[0].x[0], snakes[0].y[0], ARGB16(1, 31, 31, 31));
    }
}

static void updateConsole(void) {
    iprintf("\x1b[1;1H SNAKE DS\n\n");
    iprintf(" Score  : %d     \n", snakes[0].len - 3);
    iprintf(" Record : %d     \n\n", best);
    iprintf(" Croix : tourner\n");
    if (gameover)
        iprintf("\n GAME OVER !     \n START = rejouer\n");
    else
        iprintf("\n                 \n                \n");
}

static void newGame(void) {
    int i, f;
    for (i = 0; i < NSNAKES; i++) {
        snakes[i].alive = 0;
        respawn[i] = 0;
    }
    snakes[0].color = ARGB16(1, 4, 28, 6);
    snakes[1].color = ARGB16(1, 31, 5, 5);
    snakes[2].color = ARGB16(1, 31, 28, 4);
    snakes[3].color = ARGB16(1, 28, 6, 28);
    for (f = 0; f < MAXFOOD; f++) foodon[f] = 0;
    for (i = 0; i < NSNAKES; i++) spawnSnake(i);
    for (f = 0; f < MINFOOD; f++) placeFood(f);
    nextdx = 1;
    nextdy = 0;
    gameover = 0;
    render();
    updateConsole();
}

int main(void) {
    int frame = 0, held, down;

    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    consoleDemoInit();
    srand((unsigned int)time(NULL));
    newGame();

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        held = keysHeld();
        down = keysDown();

        if (gameover) {
            if (down & KEY_START) newGame();
            continue;
        }

        if ((held & KEY_UP) && snakes[0].dy == 0)    { nextdx = 0;  nextdy = -1; }
        if ((held & KEY_DOWN) && snakes[0].dy == 0)  { nextdx = 0;  nextdy = 1;  }
        if ((held & KEY_LEFT) && snakes[0].dx == 0)  { nextdx = -1; nextdy = 0;  }
        if ((held & KEY_RIGHT) && snakes[0].dx == 0) { nextdx = 1;  nextdy = 0;  }

        frame++;
        if (frame >= FRAMES_PER_MOVE) {
            frame = 0;
            tick();
            render();
            updateConsole();
        }
    }
    return 0;
}
