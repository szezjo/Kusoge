#ifndef PROJEKT1_SO2_CLIENT_FUNCTIONS_H
#define PROJEKT1_SO2_CLIENT_FUNCTIONS_H
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdbool.h>

sem_t *initIn;
sem_t *initOut;
int fp;
struct assign_t *pidmap;

sem_t *semIn;
sem_t *semOut;
int fd;
struct player_t *pmap;

pthread_mutex_t mtx;

struct location_t {
    int x;
    int y;
};

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

struct assign_t {
    pid_t PID;
    int slot;
    bool isHuman;
};

short graycolor;
char mapcopy[5][5];
bool quit;
struct location_t prevLoc;

void init_data();
void display_map();
void display_stats();

void *getkeys(void *nullhere);
void *autorefresh(void *nullhere);

#endif //PROJEKT1_SO2_CLIENT_FUNCTIONS_H
