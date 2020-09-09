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
#include "client_functions.h"

int main() {
    initscr();
    noecho();
    curs_set(FALSE);
    cbreak();
    start_color();
    init_data();

    initOut = sem_open("readSem_init", O_CREAT, 0600, 0);
    initIn = sem_open("bufSem_init", O_CREAT, 0600, 1);

    sem_wait(initIn);
    fp = shm_open("shared_pid", O_CREAT | O_RDWR, 0600);
    ftruncate(fp, sizeof(struct assign_t));
    pidmap = mmap(NULL, sizeof(struct assign_t), PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
    pidmap->PID=getpid();
    pidmap->isHuman=1;
    sem_post(initOut);
    sem_wait(initIn);

    if (pidmap->slot==-1) {
        mvprintw(0,0,"Error: No slots available");
        getch();
        endwin();
        noecho();
        return 0;
    }

    close(fp);
    char rname[9] = "readSemP";
    char bname[8] = "bufSemP";
    char dname[8] = "data_pP";
    rname[7]=pidmap->slot+'0';
    bname[6]=pidmap->slot+'0';
    dname[6]=pidmap->slot+'0';
    semOut = sem_open(rname, O_CREAT, 0600, 0);
    semIn = sem_open(bname, O_CREAT, 0600, 0); 
    fd = shm_open(dname, O_CREAT | O_RDWR, 0600);
    ftruncate(fd, sizeof(struct player_t));
    pmap = mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    sem_post(initIn);

    clear();
    pthread_t keys_pth;
    pthread_t refresh_pth;
    pthread_create(&keys_pth,NULL,getkeys,NULL);
    pthread_create(&refresh_pth,NULL,autorefresh,NULL);
    
    pthread_join(refresh_pth,NULL);
    pthread_join(keys_pth,NULL);

    munmap(pmap, sizeof(struct player_t));
    sem_close(semOut);
    sem_close(semIn);

    if (!quit) {
        clear();
        mvprintw(0,0,"Server disconnected");
        refresh();
        getch();
    }
    
    endwin();
    noecho();
    return 0;
}