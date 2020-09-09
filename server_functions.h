#ifndef PROJEKT1_SO2_SERVER_FUNCTIONS_H
#define PROJEKT1_SO2_SERVER_FUNCTIONS_H
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>

int debug;
bool isn;
bool quit;

sem_t *initIn;
sem_t *initOut;
int fp;
struct assign_t *pidmap;

sem_t *semIn[4];
sem_t *semOut[4];
int fd[4];
struct player_t *pmap[4];

pthread_mutex_t mtx;

struct map_t {
    char **map;
    int width;
    int height;
};

struct map_t map;
struct map_t board;

struct location_t {
    int x;
    int y;
};

struct loot_t {
    struct location_t *loc;
    int *value;
    int size;
    char ch;
};

struct loot_t loots;

struct coins_t {
    struct location_t *loc;
    int size;
    char ch;
};

struct coins_t coins;
struct coins_t ltreasures;
struct coins_t btreasures;

char grass[4];

struct beast_t {
    pthread_t beast_pth;
    struct location_t loc;
    char fov[5][5];
    char move;
    char bgrass;
    char prayer;
};

struct beast_t *beasts;
int beastsSize;
int beastSlot;

struct player_t {
    int serverPID;
    int playerPID;
    int playerNO;
    bool isHuman;
    int roundNO;
    struct location_t campLoc;
    struct location_t playerLoc;
    struct location_t spawnLoc;
    int deaths;
    int carried;
    int brought;
    char fov[5][5];

    char move;
    bool signal;
};

bool player_slots[4];
bool player_signal[4];
bool player_died[4];
struct assign_t {
    pid_t PID;
    int slot;
    bool isHuman;
};

int statx;
int roundNO;
int open_space;

struct location_t campsite;
struct location_t topleft;

void init_data();
int init_map(const char* filename, struct map_t *smap);
int display_map(struct map_t *smap);
int refresh_map();
void display_stats();

void *getkeys(void *nullhere);
void *autorefresh(void *nullhere);
struct location_t get_rand_loc();
struct location_t get_camp_loc();

int create_treasure(struct coins_t *t);
int delete_treasure(struct coins_t *t, int index);
int clear_treasures(struct coins_t *t);

int create_loot(struct loot_t *t, int value, int x, int y);
int delete_loot(struct loot_t *t, int index);

void clear_stream();

//CONNECTION FUNCTIONS
void *look_for_players(void *nullhere);
void *get_data(void *nullhere);

int create_beast();
void *move_beast(void *beastSlot);

#endif //PROJEKT1_SO2_SERVER_FUNCTIONS_H
