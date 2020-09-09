#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <string.h>
#include "server_functions.h"

#define MAPFILE "maze.txt"

int main() {
    initscr();
    noecho();
    curs_set(FALSE);
    cbreak();
    start_color();
    srand(time(0));
    isn=0;
    quit=0;

    pthread_mutex_init(&mtx, NULL);
    init_data();
    init_map(MAPFILE,&map);
    if (!open_space) {
        mvprintw(0,0,"Error: no open space");
        getch();
        endwin();
        noecho();
        return 1;
    }
    init_map(MAPFILE,&board);
    display_map(&board);
    statx=map.width+3;
    
    // Initing campsite
    campsite = get_camp_loc();
    open_space--;

    initOut = sem_open("readSem_init", O_CREAT, 0600, 0);
    initIn = sem_open("bufSem_init", O_CREAT, 0600, 1);
    fp = shm_open("shared_pid", O_CREAT | O_RDWR, 0600);
    ftruncate(fp, sizeof(struct assign_t));
    pidmap = mmap(NULL, sizeof(struct assign_t), PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);


    pthread_t refresh_pth;
    pthread_t keys_pth;
    pthread_t scan_pth;
    pthread_t getdata_pth;
    pthread_create(&refresh_pth,NULL,autorefresh,NULL);
    pthread_create(&keys_pth,NULL,getkeys,NULL);
    pthread_create(&scan_pth,NULL,look_for_players,NULL);
    pthread_create(&getdata_pth,NULL,get_data,NULL);
    pthread_join(keys_pth,NULL);
    pthread_join(refresh_pth,NULL);
    sem_post(initOut);
    pthread_join(scan_pth,NULL);
    pthread_join(getdata_pth,NULL);

    if(map.map) {
        for(int i=0; i<=map.height; i++) free(map.map[i]);
        free(map.map);
    }
    if(board.map) {
        for(int i=0; i<=board.height; i++) free(board.map[i]);
        free(board.map);
    }
    if(coins.loc) free(coins.loc);
    if(ltreasures.loc) free(ltreasures.loc);
    if(btreasures.loc) free(btreasures.loc);
    if(loots.loc) free(loots.loc);
    if(loots.value) free(loots.value);
    for(int i=0; i<beastsSize; i++) {
        pthread_join(beasts[i].beast_pth,NULL);
    }
    if(beasts) free(beasts);

    munmap(pidmap, sizeof(struct assign_t));
    sem_close(initIn);
    sem_close(initOut);
    endwin();
    noecho();
    return 0;
}
