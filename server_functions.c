#include <ncurses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include "server_functions.h"

#define RETBADLOC { y=-1; x=-1; break; }
#define PINFO statx+12+i*9

void init_data() {
    for (int i=0; i<4; i++) player_slots[i]=0;
    coins.size=0;
    coins.loc=NULL;
    ltreasures.size=0;
    ltreasures.loc=NULL;
    btreasures.size=0;
    btreasures.loc=NULL;
    loots.loc=NULL;
    loots.value=NULL;
    loots.size=0;

    coins.ch='c';
    ltreasures.ch='t';
    btreasures.ch='T';
    loots.ch='D';
    beasts=NULL;
    beastsSize=0;

    for (int i=0; i<4; i++) grass[i]=0;
    for (int i=0; i<4; i++) player_died[i]=0;

    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK); //bushes

    //Markers
    init_pair(10, COLOR_YELLOW, COLOR_MAGENTA); //campsite
    init_pair(11, COLOR_BLACK, COLOR_YELLOW); //coins and treasures
    init_pair(12, COLOR_GREEN, COLOR_YELLOW); //loots
    init_pair(13, COLOR_WHITE, COLOR_MAGENTA); //players
    init_pair(14, COLOR_RED, COLOR_BLACK); //beast
}

int init_map(const char* filename, struct map_t *smap) {
    if (!filename || !smap) return 1;
    FILE *f;
    f = fopen(filename, "r");
    if (!f) return 2;
    

    int width=0;
    int height=0;
    int tmp=0;
    open_space=0;
    move(5,0);

    char c;
    fscanf(f,"%c", &c);
    while (!feof(f)) {
        if (c!='\n') {
            if (c!='\r') tmp++;
            fscanf(f,"%c",&c);
        }
        else {
            if (tmp>width) width=tmp;
            height++;
            tmp=0;
            fscanf(f,"%c",&c);
        }
    }

    debug=c;
    if (c!='\n') {
        height++;
        isn=1;
    }
    char **temp = (char**)malloc(sizeof(char*)*(height+1));
    if (!temp) return 6;
    for (int i=0; i<=height; i++) {
        char *tempin = (char*)malloc(sizeof(char)*width);
        if (!tempin) {
            for (int j=0; j<i; j++) {
                free(*(temp+j));
            }
            free(temp);
            return 6;
        }
        *(temp+i)=tempin;
    }

    fseek(f,0,SEEK_SET);
    fscanf(f,"%c", &c);
    int i=0;
    int j=0;
    while (!feof(f)) {
        if (c!='\n') {
            *(*(temp+i)+j)=c;
            j++;
            if(c==' ' || c=='w') open_space++;
            fscanf(f,"%c",&c);
        }
        else if (c!='\r') {
            *(*(temp+i)+j)=c;
            i++;
            j=0;
            fscanf(f,"%c",&c);
        }
    }
    fseek(f,0,SEEK_SET);
    fscanf(f,"%c",&c);


    smap->map=temp;
    smap->width=width;
    smap->height=height;
    fclose(f);
    return 0;
}

int display_map(struct map_t *smap) {
    if (!smap || !smap->map || !smap->width || !smap->height) return 1;
    move(0,0);
    for (int i=0; i<smap->height; i++) {
        for (int j=0; j<smap->width; j++) {
            if (*(*(smap->map+i)+j)=='#') {
                attron(COLOR_PAIR(1));
                printw(" ");
                attroff(COLOR_PAIR(1));
            }
            else if (*(*(smap->map+i)+j)==' ') {
                attron(COLOR_PAIR(2));
                printw(" ");
                attroff(COLOR_PAIR(2));
            }
            else if (*(*(smap->map+i)+j)=='w') {
                attron(COLOR_PAIR(3));
                printw("#");
                attroff(COLOR_PAIR(3));
            }
            else if (*(*(smap->map+i)+j)=='A') {
                attron(COLOR_PAIR(10));
                printw("A");
                attroff(COLOR_PAIR(10));
            }
            else if (*(*(smap->map+i)+j)==coins.ch || *(*(smap->map+i)+j)==ltreasures.ch || *(*(smap->map+i)+j)==btreasures.ch) {
                attron(COLOR_PAIR(11));
                printw("%c",*(*(smap->map+i)+j));
                attroff(COLOR_PAIR(11));
            }
            else if (*(*(smap->map+i)+j)==loots.ch) {
                attron(COLOR_PAIR(12));
                printw("%c",*(*(smap->map+i)+j));
                attroff(COLOR_PAIR(12));
            }
            else if (*(*(smap->map+i)+j)-'0'>=1 && *(*(smap->map+i)+j)-'0'<=4) {
                attron(COLOR_PAIR(13));
                printw("%c",*(*(smap->map+i)+j));
                attroff(COLOR_PAIR(13));
            }
            else if (*(*(smap->map+i)+j)=='*') {
                attron(COLOR_PAIR(14));
                printw("*");
                attroff(COLOR_PAIR(14));
            }
            else if (*(*(smap->map+i)+j)=='\n') break;
        }
        move(i+1, 0);
    }
    return 0;
}

void display_stats() {
    pid_t srvpid = getpid();
    mvprintw(1,statx,"Server's PID: %d",srvpid);
    mvprintw(2,statx,"Campsite X/Y: %d %d",campsite.x,campsite.y);
    mvprintw(3,statx,"Round number: %d",roundNO);
    mvprintw(5,statx,"Parameter:");
    mvprintw(6,statx," PID");
    mvprintw(7,statx," Type");
    mvprintw(8,statx," Curr X/Y");
    mvprintw(9,statx," Deaths");
    mvprintw(11,statx,"Coins");
    mvprintw(12,statx,"    carried");
    mvprintw(13,statx,"    brought");

    for (int i=0; i<4; i++) {
        if (player_slots[i]) {
            mvprintw(5,PINFO,"Player%d",pmap[i]->playerNO+1);
            mvprintw(6,PINFO,"%d",pmap[i]->playerPID);
            if (pmap[i]->isHuman) mvprintw(7,PINFO,"HUMAN");
            else mvprintw(7,PINFO,"BOT");
            mvprintw(8,PINFO,"%d/%d",pmap[i]->playerLoc.x,pmap[i]->playerLoc.y);
            mvprintw(9,PINFO,"%d",pmap[i]->deaths);
            mvprintw(12,PINFO,"%d",pmap[i]->carried);
            mvprintw(13,PINFO,"%d",pmap[i]->brought);
        }
    }

}

void clear_markers() {
    for (int i=0; i<map.height; i++) {
        for (int j=0; j<map.width; j++) {
            board.map[i][j]=map.map[i][j];
        }
    }
}

int refresh_map() {
    if (!map.width || !map.height || !map.map || !board.width || !board.height || !board.map || board.height!=map.height || board.width!=map.width) return 1;
    clear_markers();
    board.map[campsite.y][campsite.x]='A';
    
    for (int i=0; i<coins.size; i++) {
        board.map[coins.loc[i].y][coins.loc[i].x]=coins.ch;
    }

    for (int i=0; i<ltreasures.size; i++) {
        board.map[ltreasures.loc[i].y][ltreasures.loc[i].x]=ltreasures.ch;
    }

    for (int i=0; i<btreasures.size; i++) {
        board.map[btreasures.loc[i].y][btreasures.loc[i].x]=btreasures.ch;
    }


    for (int i=0; i<4; i++) {
        if (player_slots[i]) {
            board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]=i+1+'0';
        }
    }

    for (int i=0; i<beastsSize; i++) {
        board.map[beasts[i].loc.y][beasts[i].loc.x]='*';
    }

    for (int i=0; i<loots.size; i++) {
        board.map[loots.loc[i].y][loots.loc[i].x]=loots.ch;
    }
    
    for (int i=0; i<4; i++) {
        if (player_slots[i]) {
            topleft.x=pmap[i]->playerLoc.x-2;
            topleft.y=pmap[i]->playerLoc.y-2;
            for (int j=0; j<5; j++) {
                for (int k=0; k<5; k++) {
                    if (topleft.x+k<0 || topleft.x+k>=map.width || topleft.y+j<0 || topleft.y+j>=map.height) pmap[i]->fov[j][k]='X';         
                    else {
                        pmap[i]->fov[j][k]=board.map[topleft.y+j][topleft.x+k];
                        if(board.map[topleft.y+j][topleft.x+k]=='A') {
                            pmap[i]->campLoc=campsite;
                        }
                    }
                }
            }
            if (player_died[i]) {
                pmap[i]->fov[2][2]='+';
                player_died[i]=0;
            }
        }
    }

    for (int i=0; i<beastsSize; i++) {
        topleft.x=beasts[i].loc.x-2;
        topleft.y=beasts[i].loc.y-2;
        for (int j=0; j<5; j++) {
            for (int k=0; k<5; k++) {
                if (topleft.x+k<0 || topleft.x+k>=map.width || topleft.y+j<0 || topleft.y+j>=map.height) beasts[i].fov[j][k]='#';         
                else {
                    beasts[i].fov[j][k]=board.map[topleft.y+j][topleft.x+k];
                }
            }
        }
    }
    return 0;
}

struct location_t get_rand_loc() {
    struct location_t tmp;
    int x,y;
    if (!open_space) {
        tmp.x=-1;
        tmp.y=-1;
        return tmp;
    }
    while(1) {
        x=rand()%board.width;
        y=rand()%board.height;
        if(board.map[y][x]==' ' || board.map[y][x]=='w') break;
    }
    tmp.x=x;
    tmp.y=y;
    return tmp;
}

struct location_t get_camp_loc() {
    struct location_t tmp;
    int x,y;
    x=board.width/2;
    y=board.height/2;
    int i=0;
    move(30,0);
    if(board.map[y][x]!=' ') {
        while (1) {
            printw("%d %d | ",y,x);
            i++;
            if(y+i<board.height) { if(board.map[y+i][x]==' ' || board.map[y+i][x]=='w') {
                y=y+i;
                break;
            }
            }
            else RETBADLOC
            if(y-i>=0) { if(board.map[y-i][x]==' ' || board.map[y-i][x]=='w') {
                y=y-i;
                break;
            }
            }
            else RETBADLOC
            if(x+i<board.width) { if(board.map[y][x+i]==' ' || board.map[y][x+i]=='w') {
                x=x+i;
                break;
            } 
            }
            else RETBADLOC
            if(x-i>=0) { if(board.map[y][x-i]==' ' || board.map[y][x-i]=='w') {
                x=x-i;
                break;
            }
            }
            else RETBADLOC
            if(board.map[y+i][x+i]==' ' || board.map[y+i][x+i]=='w') {
                y=y+i;
                x=x+i;
                break;
            }
            if(board.map[y+i][x-i]==' ' || board.map[y+i][x-i]=='w') {
                y=y+i;
                x=x-i;
                break;
            }
            if(board.map[y-i][x+i]==' ' || board.map[y-i][x+i]=='w') {
                y=y-i;
                x=x+i;
                break;
            }
            if(board.map[y-i][x-i]==' ' || board.map[y-i][x-i]=='w') {
                y=y-i;
                x=x-i;
                break;
            }
        }
    }
    tmp.x=x;
    tmp.y=y;
    return tmp;
}

int create_treasure(struct coins_t *t) {
    pthread_mutex_lock(&mtx);
    struct location_t loc=get_rand_loc();
    if (loc.x==-1 || loc.y==-1) {
        pthread_mutex_unlock(&mtx);
        return 7;
    }
    struct location_t *temp=(struct location_t *)realloc(t->loc,sizeof(struct location_t)*(t->size+1));
    if (!temp) return 6;
    temp[t->size].x=loc.x;
    temp[t->size].y=loc.y;
    t->size++;
    t->loc=temp;
    board.map[loc.y][loc.x]=t->ch;
    open_space--;
    pthread_mutex_unlock(&mtx);
    return 0;
}

int delete_treasure(struct coins_t *t, int index) {
    if (!t || !t->size || !t->loc) return 1;
    if (index>=t->size) return 2;

    for (int i=index; i<t->size-1; i++) {
        t->loc[i]=t->loc[i+1];
    }

    if (t->size==1) {
        free(t->loc);
        t->loc=NULL;
        t->size=0;
    }
    else {
        struct location_t *temp=(struct location_t *)realloc(t->loc,sizeof(struct location_t)*(t->size-1));
        if (!temp) return 6;
        t->size--;
        t->loc=temp;
    }
    open_space++;
    return 0;
}

int create_loot(struct loot_t *t, int value, int x, int y) {
    struct location_t *temp=(struct location_t *)realloc(t->loc,sizeof(struct location_t)*(t->size+1));
    if (!temp) {
        return 6;
    }
    int *tmpv=(int*)realloc(t->value, sizeof(int)*(t->size+1));
    if (!tmpv) {
        free(temp);
        return 6;
    }
    temp[t->size].x=x;
    temp[t->size].y=y;
    tmpv[t->size]=value;
    t->size++;
    t->loc=temp;
    t->value=tmpv;
    board.map[y][x]=t->ch;
    open_space--;
    return 0;
}

int delete_loot(struct loot_t *t, int index) {
    if (!t || !t->size || !t->loc) return 1;
    if (index>=t->size) return 2;

    for (int i=index; i<t->size-1; i++) {
        t->loc[i]=t->loc[i+1];
        t->value[i]=t->value[i+1];
    }

    if (t->size==1) {
        free(t->loc);
        free(t->value);
        t->loc=NULL;
        t->value=NULL;
        t->size=0;
    }
    else {
        struct location_t *temp=(struct location_t *)realloc(t->loc,sizeof(struct location_t)*(t->size-1));
        if (!temp) return 6;
        int *tmpv=(int *)realloc(t->value,sizeof(int)*(t->size-1));
        if (!tmpv) {
            free(temp);
            return 6;
        }
        t->size--;
        t->loc=temp;
        t->value=tmpv;
    }
    open_space++;
    return 0;
}

int clear_treasures(struct coins_t *t) {
    int err;
    int n=t->size;
    for (int i=0; i<n; i++) {
        err = delete_treasure(t,0);
        if (err) return err;
    }
    return 0;
}

void *getkeys(void *nullhere) {
    int ch;
    while(1) {
        ch=getch();
        if (ch=='c') {
            create_treasure(&coins);
        }
        else if (ch=='t') {
            create_treasure(&ltreasures);
        }
        else if (ch=='T') {
            create_treasure(&btreasures);
        }
        else if (ch=='b' || ch=='B') {
            create_beast();
        }
        else if (ch=='1') {
            pthread_mutex_lock(&mtx);
            delete_treasure(&coins,1);
            pthread_mutex_unlock(&mtx);
        }
        else if (ch=='0') {
            pthread_mutex_lock(&mtx);
            clear_treasures(&coins);
            pthread_mutex_unlock(&mtx);
        }
        else if (ch=='q') {
            quit=1;
            return NULL;
        }
    }
}

void *autorefresh(void *nullhere) {
    while(1) {
        pthread_mutex_lock(&mtx);
        refresh_map();
        clear();
        display_map(&board);
        display_stats();
        pthread_mutex_unlock(&mtx);
        
        if(quit) break;
        usleep(100000);
        refresh();
    }
    return NULL;
}

void clear_stream() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void *look_for_players(void *nullhere) {
    while(1) {
        if(quit) break;
        sem_wait(initOut);
        if(quit) break;
        pthread_mutex_lock(&mtx);

        int slot=-1;
        for (int i=0; i<4; i++) {
            if (!player_slots[i]) {
                slot=i;
                break;
            }
        }

        pidmap->slot=slot;
        
        if (slot!=-1) {
                char rname[9] = "readSemP";
                char bname[8] = "bufSemP";
                char dname[8] = "data_pP";
                rname[7]=slot+'0';
                bname[6]=slot+'0';
                dname[6]=slot+'0';
                struct location_t spawn;
                bool spawnDone=0;
                while(!spawnDone) {
                    spawn = get_rand_loc();
                    spawnDone=1;
                    for (int i=0; i<4; i++) {
                        if (player_slots[i] && i!=slot) {
                            if (spawn.y == pmap[i]->playerLoc.y && spawn.x == pmap[i]->playerLoc.x) {
                                spawnDone=0;
                                break;
                            }
                        }
                    }
                }

                semOut[slot] = sem_open(rname, O_CREAT, 0600, 0);
                semIn[slot] = sem_open(bname, O_CREAT, 0600, 0);
                fd[slot] = shm_open(dname, O_CREAT | O_RDWR, 0600);
                ftruncate(fd[slot], sizeof(struct player_t));
                pmap[slot] = mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd[slot], 0);
                pmap[slot]->serverPID=getpid();
                pmap[slot]->playerPID=pidmap->PID;
                pmap[slot]->playerNO=slot;
                pmap[slot]->isHuman=pidmap->isHuman;
                pmap[slot]->roundNO=roundNO;
                pmap[slot]->campLoc.x=-1;
                pmap[slot]->campLoc.y=-1;
                pmap[slot]->spawnLoc=spawn;
                pmap[slot]->playerLoc=spawn;
                pmap[slot]->deaths=0;
                pmap[slot]->carried=0;
                pmap[slot]->brought=0;
                pmap[slot]->move='x';
                pmap[slot]->signal=0;
                player_slots[slot]=1;
        }
        pthread_mutex_unlock(&mtx);
        sem_post(initIn);
        if(quit) break;
    }
    return NULL;
}

void *get_data(void *nullhere) {
    while(1) {
        pthread_mutex_lock(&mtx);

        for (int i=0; i<beastsSize; i++) {
            if (beasts[i].bgrass==1) {
                    beasts[i].bgrass=2;
                    break;
                }
                if (beasts[i].move=='w') {
                    if (board.map[beasts[i].loc.y-1][beasts[i].loc.x]!='#' && beasts[i].loc.y>0) beasts[i].loc.y--;
                    beasts[i].bgrass=0;
                }
                else if (beasts[i].move=='s') {
                    if (board.map[beasts[i].loc.y+1][beasts[i].loc.x]!='#' && beasts[i].loc.y<board.height) beasts[i].loc.y++;
                    beasts[i].bgrass=0;
                }
                else if (beasts[i].move=='a') {
                    if (board.map[beasts[i].loc.y][beasts[i].loc.x-1]!='#' && beasts[i].loc.x>0) beasts[i].loc.x--;
                    beasts[i].bgrass=0;
                }
                else if (beasts[i].move=='d') {
                    if (board.map[beasts[i].loc.y][beasts[i].loc.x+1]!='#' && beasts[i].loc.x<board.width) beasts[i].loc.x++;
                    beasts[i].bgrass=0;
                }
                if (beasts[i].bgrass!=1) beasts[i].move='x';
        }
        refresh_map();

        for (int i=0; i<4; i++) {
            if (player_slots[i]) {
                if (kill(pmap[i]->playerPID, 0) || pmap[i]->move=='q') {
                    player_slots[i]=0;
                    munmap(pmap[i], sizeof(struct player_t));
                    sem_close(semOut[i]);
                    sem_close(semIn[i]);

                    char rname[9] = "readSemP";
                    char bname[8] = "bufSemP";
                    char dname[8] = "data_pP";
                    rname[7]=i+'0';
                    bname[6]=i+'0';
                    dname[6]=i+'0';

                    sem_unlink(rname);
                    sem_unlink(bname);
                    shm_unlink(dname);

                    break;
                }
                if (grass[i]==1) {
                    grass[i]=2;
                    break;
                }
                if (pmap[i]->move=='w') {
                    if (board.map[pmap[i]->playerLoc.y-1][pmap[i]->playerLoc.x]!='#' && pmap[i]->playerLoc.y>0) pmap[i]->playerLoc.y--;
                    grass[i]=0;
                }
                else if (pmap[i]->move=='s') {
                    if (board.map[pmap[i]->playerLoc.y+1][pmap[i]->playerLoc.x]!='#' && pmap[i]->playerLoc.y<board.height) pmap[i]->playerLoc.y++;
                    grass[i]=0;
                }
                else if (pmap[i]->move=='a') {
                    if (board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x-1]!='#' && pmap[i]->playerLoc.x>0) pmap[i]->playerLoc.x--;
                    grass[i]=0;
                }
                else if (pmap[i]->move=='d') {
                    if (board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x+1]!='#' && pmap[i]->playerLoc.x<board.width) pmap[i]->playerLoc.x++;
                    grass[i]=0;
                }
                if (grass[i]!=1) pmap[i]->move='x';

                if (map.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]=='w' && !grass[i]) grass[i]=1;
                
                if (board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]==coins.ch) {
                    for (int j=0; j<coins.size; j++) {
                        if (pmap[i]->playerLoc.y==coins.loc[j].y && pmap[i]->playerLoc.x==coins.loc[j].x) {
                            pmap[i]->carried++;
                            board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]=board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x];
                            delete_treasure(&coins, j);
                            break;
                        }
                    }
                }

                else if (board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]==ltreasures.ch) {
                    for (int j=0; j<ltreasures.size; j++) {
                        if (pmap[i]->playerLoc.y==ltreasures.loc[j].y && pmap[i]->playerLoc.x==ltreasures.loc[j].x) {
                            pmap[i]->carried+=10;
                            board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]=board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x];
                            delete_treasure(&ltreasures, j);
                            break;
                        }
                    }
                }

                else if (board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]==btreasures.ch) {
                    for (int j=0; j<btreasures.size; j++) {
                        if (pmap[i]->playerLoc.y==btreasures.loc[j].y && pmap[i]->playerLoc.x==btreasures.loc[j].x) {
                            pmap[i]->carried+=50;
                            board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]=board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x];
                            delete_treasure(&btreasures, j);
                            break;
                        }
                    }
                }

                else if (board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]==loots.ch) {
                    for (int j=0; j<loots.size; j++) {
                        if (pmap[i]->playerLoc.y==loots.loc[j].y && pmap[i]->playerLoc.x==loots.loc[j].x) {
                            pmap[i]->carried+=loots.value[j];
                            board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]=board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x];
                            delete_loot(&loots, j);
                            break;
                        }
                    }
                }

                for (int j=0; j<4; j++) {
                    if (player_slots[j] && j!=i) {
                        if (pmap[i]->playerLoc.y==pmap[j]->playerLoc.y && pmap[i]->playerLoc.x==pmap[j]->playerLoc.x) {
                            int val = pmap[i]->carried;
                            val+=pmap[j]->carried;
                            pmap[i]->carried=0;
                            pmap[j]->carried=0;
                            if (val) create_loot(&loots, val, pmap[i]->playerLoc.x, pmap[i]->playerLoc.y);
                            pmap[i]->playerLoc.y=pmap[i]->spawnLoc.y;
                            pmap[i]->playerLoc.x=pmap[i]->spawnLoc.x;
                            pmap[j]->playerLoc.y=pmap[j]->spawnLoc.y;
                            pmap[j]->playerLoc.x=pmap[j]->spawnLoc.x;
                            break;
                        }
                    }
                }

                if (pmap[i]->playerLoc.y == campsite.y && pmap[i]->playerLoc.x == campsite.x) {
                    pmap[i]->brought+=pmap[i]->carried;
                    pmap[i]->carried=0;
                }

                refresh_map();

                if (board.map[pmap[i]->playerLoc.y][pmap[i]->playerLoc.x]=='*') {
                    if (pmap[i]->carried) create_loot(&loots, pmap[i]->carried, pmap[i]->playerLoc.x, pmap[i]->playerLoc.y);
                    pmap[i]->carried=0;
                    pmap[i]->deaths++;
                    player_died[i]=1;
                    refresh_map();
                    usleep(200000);
                    pmap[i]->playerLoc.y=pmap[i]->spawnLoc.y;
                    pmap[i]->playerLoc.x=pmap[i]->spawnLoc.x;
                    for (int j=0; j<beastsSize; j++) {
                        if (beasts[j].prayer==pmap[i]->playerNO+1+'0') beasts[j].prayer='x';
                    }
                    refresh_map();
                }
            }

        }

        pthread_mutex_unlock(&mtx);
        if(quit) break;
        usleep(400000);
    }
    return NULL;
}

int create_beast() {
    pthread_mutex_lock(&mtx);
    struct location_t loc=get_rand_loc();
    if (loc.x==-1 || loc.y==-1) {
        pthread_mutex_unlock(&mtx);
        return 7;
    }
    struct beast_t *temp=(struct beast_t *)realloc(beasts,sizeof(struct beast_t)*(beastsSize+1));
    if (!temp) return 6;
    temp[beastsSize].loc=loc;
    temp[beastsSize].move='x';
    beastsSize++;
    beasts=temp;
    board.map[loc.y][loc.x]='*';
    open_space--;
    pthread_mutex_unlock(&mtx);
    pthread_create(&(beasts[beastsSize-1].beast_pth), NULL, move_beast, (void*)&beastsSize);
    return 0;
}

int rotate_cw(int dir) {
    if (dir<3) return dir+1;
    else return 0;
}

void *move_beast(void *beastSlot) {
    int where;
    int slot = *(int*)beastSlot-1;
    struct location_t prayerLoc;
    char moves[5] = {'w','d','s','a','x'};
    int y_gap;
    int x_gap;
    while(1) {
        pthread_mutex_lock(&mtx);
        refresh_map();

        // check if there's any yummy pray
        
        if (beasts[slot].prayer=='x') {
            for (int i=0; i<5; i++) {
                for (int j=0; j<5; j++) {
                    if (beasts[slot].fov[i][j]>=0+'0' && beasts[slot].fov[i][j]<=3+'0') {
                        if ((i>=1 && i<=3) && (j>=1 && j<=3)) {
                            beasts[slot].prayer=beasts[slot].fov[i][j];
                            prayerLoc.x=j+beasts[slot].loc.x-2;
                            prayerLoc.y=i+beasts[slot].loc.y-2;
                            break;
                        }
                        else if (i==2) {
                            if ((j==0 && beasts[slot].fov[2][1]!='#') || (j==4 && beasts[slot].fov[2][3]!='#')) {
                                beasts[slot].prayer=beasts[slot].fov[i][j];
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }   
                        }
                        else if (j==2) {
                            if ((i==0 && beasts[slot].fov[1][2]!='#') || (i==4 && beasts[slot].fov[3][2]!='#')) {
                                beasts[slot].prayer=beasts[slot].fov[i][j];
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }  
                        }
                        else if (i<=1 && j<=1) {
                            if (beasts[slot].fov[1][1]!='#') {
                                beasts[slot].prayer=beasts[slot].fov[i][j];
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                        }
                        else if (i<=1 && j>=3) {
                            if (beasts[slot].fov[1][3]!='#') {
                                beasts[slot].prayer=beasts[slot].fov[i][j];
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2; 
                                break;
                            }
                        }
                        else if (i>=3 && j<=1) {
                            if (beasts[slot].fov[3][1]!='#') {
                                beasts[slot].prayer=beasts[slot].fov[i][j];
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                        }
                        else if (i>=3 && j>=3) {
                            if (beasts[slot].fov[3][3]!='#') {
                                beasts[slot].prayer=beasts[slot].fov[i][j];
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                        }

                    }
                }
            }
        }

        else {
            for (int i=0; i<5; i++) {
                for (int j=0; j<5; j++) {
                    if (beasts[slot].fov[i][j]==beasts[slot].prayer) {
                        if ((i>=1 && i<=3) && (j>=1 && j<=3)) {
                            prayerLoc.x=j+beasts[slot].loc.x-2;
                            prayerLoc.y=i+beasts[slot].loc.y-2;
                            break;
                        }
                        else if (i==2) {
                            if ((j==0 && beasts[slot].fov[2][1]!='#') || (j==4 && beasts[slot].fov[2][3]!='#')) {
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                            else {
                                beasts[slot].prayer='x';
                                prayerLoc.x=-1;
                                prayerLoc.y=-1;
                                break;
                            }
                        }
                        else if (j==2) {
                            if ((i==0 && beasts[slot].fov[1][2]!='#') || (i==4 && beasts[slot].fov[3][2]!='#')) {
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                            else {
                                beasts[slot].prayer='x';
                                prayerLoc.x=-1;
                                prayerLoc.y=-1;
                                break;
                            }  
                        }
                        else if (i<=1 && j<=1) {
                            if (beasts[slot].fov[1][1]!='#') {
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                            else {
                                beasts[slot].prayer='x';
                                prayerLoc.x=-1;
                                prayerLoc.y=-1;
                                break;
                            }
                        }
                        else if (i<=1 && j>=3) {
                            if (beasts[slot].fov[1][3]!='#') {
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2; 
                                break;
                            }
                            else {
                                beasts[slot].prayer='x';
                                prayerLoc.x=-1;
                                prayerLoc.y=-1;
                                break;
                            }
                        }
                        else if (i>=3 && j<=1) {
                            if (beasts[slot].fov[3][1]!='#') {
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                            else {
                                beasts[slot].prayer='x';
                                prayerLoc.x=-1;
                                prayerLoc.y=-1;
                                break;
                            }
                        }
                        else if (i>=3 && j>=3) {
                            if (beasts[slot].fov[3][3]!='#') {
                                prayerLoc.x=j+beasts[slot].loc.x-2;
                                prayerLoc.y=i+beasts[slot].loc.y-2;
                                break;
                            }
                            else {
                                beasts[slot].prayer='x';
                                prayerLoc.x=-1;
                                prayerLoc.y=-1;
                                break;
                            }
                        }

                    }
                }
            }
        }

        if (beasts[slot].prayer!='x') {
            y_gap=beasts[slot].loc.y-prayerLoc.y;
            if (y_gap<0) y_gap*=-1;
            x_gap=beasts[slot].loc.x-prayerLoc.x;
            if (x_gap<0) x_gap*=-1;

            if (y_gap>=x_gap) {
                if (beasts[slot].loc.y <= prayerLoc.y && beasts[slot].fov[3][2]!='#') where=2;
                else if (beasts[slot].loc.y >= prayerLoc.y && beasts[slot].fov[1][2]!='#') where=0;
                else if (beasts[slot].loc.x <= prayerLoc.x && beasts[slot].fov[2][3]!='#') where=1;
                else if (beasts[slot].loc.x >= prayerLoc.x && beasts[slot].fov[2][1]!='#') where=3;
                else beasts[slot].prayer='x';
            }

            else {
                if (beasts[slot].loc.x <= prayerLoc.x && beasts[slot].fov[2][3]!='#') where=1;
                else if (beasts[slot].loc.x >= prayerLoc.x && beasts[slot].fov[2][1]!='#') where=3;
                else if (beasts[slot].loc.y <= prayerLoc.y && beasts[slot].fov[3][2]!='#') where=2;
                else if (beasts[slot].loc.y >= prayerLoc.y && beasts[slot].fov[1][2]!='#') where=0;
                else beasts[slot].prayer='x';
            }  
        }

        else {
            where=rand()%10;
            if (where/2==0 && beasts[slot].fov[1][2]=='#') where=rotate_cw(where/2)*2;
            if (where/2==1 && beasts[slot].fov[2][3]=='#') where=rotate_cw(where/2)*2;
            if (where/2==2 && beasts[slot].fov[3][2]=='#') where=rotate_cw(where/2)*2;
            if (where/2==3 && beasts[slot].fov[2][1]=='#') where=rotate_cw(where/2)*2;

            where/=2;
        }
        

        beasts[slot].move=moves[where];
        pthread_mutex_unlock(&mtx);
        if(quit) break;
        usleep(400000);
    }
    return NULL;
}

