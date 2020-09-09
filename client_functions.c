#include <ncurses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include "client_functions.h"

#define MAXSIZE 128
#define COLOR_GRAY 10

void init_data() {
    quit=0;
    init_pair(1, COLOR_BLACK, COLOR_WHITE); //wall
    init_pair(2, COLOR_WHITE, COLOR_BLACK); //open space
    init_pair(3, COLOR_GREEN, COLOR_BLACK); //bushes

    //Markers
    init_pair(10, COLOR_YELLOW, COLOR_MAGENTA); //campsite
    init_pair(11, COLOR_BLACK, COLOR_YELLOW); //coins and treasures
    init_pair(12, COLOR_GREEN, COLOR_YELLOW); //loots
    init_pair(13, COLOR_WHITE, COLOR_MAGENTA); //players
    init_pair(14, COLOR_RED, COLOR_BLACK); //beast
    

    if (COLORS>8) {
        init_color(COLOR_GRAY, 500,500,500);
        graycolor=COLOR_GRAY;
    }
    else {
        init_color(COLOR_CYAN, 500,500,500);
        graycolor=COLOR_CYAN;
    }

    init_pair(20, COLOR_BLACK, graycolor); //gray walls
}

void display_map() {
    pthread_mutex_lock(&mtx);

    for (int i=0; i<5; i++) {
        for (int j=0; j<5; j++) {
            if (mapcopy[i][j]=='#') {
                attron(COLOR_PAIR(20));
                mvprintw(prevLoc.y-2+i,prevLoc.x-2+j," ");
                attroff(COLOR_PAIR(20));
            }
        }
    }

    for (int i=0; i<5; i++) {
        for (int j=0; j<5; j++) {
            mapcopy[i][j]=pmap->fov[i][j];
        }
    }
    prevLoc=pmap->playerLoc;

    
    for (int i=0; i<5; i++) {
        for (int j=0; j<5; j++) {
            if (pmap->fov[i][j]=='#') {
                attron(COLOR_PAIR(1));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j," ");
                attroff(COLOR_PAIR(1));
            }
            else if (pmap->fov[i][j]==' ') {
                attron(COLOR_PAIR(2));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j," ");
                attroff(COLOR_PAIR(2));
            }
            else if (pmap->fov[i][j]=='w') {
                attron(COLOR_PAIR(3));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j,"#");
                attroff(COLOR_PAIR(3));
            }
            else if (pmap->fov[i][j]=='A') {
                attron(COLOR_PAIR(10));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j,"A");
                attroff(COLOR_PAIR(10));
            }
            else if (pmap->fov[i][j]=='c' || pmap->fov[i][j]=='t' || pmap->fov[i][j]=='T') {
                attron(COLOR_PAIR(11));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j,"%c",pmap->fov[i][j]);
                attroff(COLOR_PAIR(11));
            }
            else if (pmap->fov[i][j]=='D') {
                attron(COLOR_PAIR(12));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j,"D");
                attroff(COLOR_PAIR(12));
            }
            else if (pmap->fov[i][j]-'0'>=1 && pmap->fov[i][j]-'0'<=4) {
                attron(COLOR_PAIR(13));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j,"%c",pmap->fov[i][j]);
                attroff(COLOR_PAIR(13));
            }
            else if (pmap->fov[i][j]=='*') {
                attron(COLOR_PAIR(14));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j,"*");
                attroff(COLOR_PAIR(14));
            }
            else if (pmap->fov[i][j]=='+') {
                attron(COLOR_PAIR(20));
                mvprintw(pmap->playerLoc.y-2+i,pmap->playerLoc.x-2+j,"+");
                attroff(COLOR_PAIR(20));
            }
        }
    }
    pthread_mutex_unlock(&mtx);
    
}

void display_stats() {
    pthread_mutex_lock(&mtx);
    mvprintw(1,MAXSIZE+2,"Server's PID: %d    ", pmap->serverPID);
    if (pmap->campLoc.x==-1 && pmap->campLoc.y==-1) mvprintw(2,MAXSIZE+2,"Campsite X/Y: unknown");
    else mvprintw(2,MAXSIZE+2,"Campsite X/Y: %d/%d   ", pmap->campLoc.x, pmap->campLoc.y);
    mvprintw(3,MAXSIZE+2,"Round number: %d    ",pmap->roundNO);
    mvprintw(5,MAXSIZE+2,"Player:");
    mvprintw(6,MAXSIZE+2," Number: %d",pmap->playerNO+1);
    if (pmap->isHuman) mvprintw(7,MAXSIZE+2," Type: HUMAN");
    else mvprintw(7,MAXSIZE+2," Type: BOT");
    mvprintw(8,MAXSIZE+2," Curr X/Y: %d/%d   ", pmap->playerLoc.x, pmap->playerLoc.y);
    mvprintw(9,MAXSIZE+2," Deaths: %d   ", pmap->deaths);
    mvprintw(11,MAXSIZE+2,"Coins carried: %d        ", pmap->carried);
    mvprintw(12,MAXSIZE+2,"Coins brought: %d        ", pmap->brought);
    
    pthread_mutex_unlock(&mtx);
}

void *getkeys(void *nullhere) {
    int ch;
    while(1) {
        ch=getch();
        if (ch=='w') {
            pmap->move='w';
        }
        else if (ch=='s') {
            pmap->move='s';
        }
        else if (ch=='a') {
            pmap->move='a';
        }
        else if (ch=='d') {
            pmap->move='d';
        }
        else if (ch=='q') {
            pmap->move='q';
            quit=1;
            break;
        }
    }
return NULL;
}

void *autorefresh(void *nullhere) {
    while(!quit) {
        if(kill(pmap->serverPID,0)) {
            break;
        }   
        display_map();
        display_stats();
        

        usleep(100000);
        refresh();
    }
    return NULL;
}