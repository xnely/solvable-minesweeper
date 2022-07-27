#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

// void solvable(int argc, const char **argv){
//     printf("Hello from C!\nWe are finally free from the hell that is vala.\nSurely it is only uphill from here!\n");
//     for(int i=0; i<argc; ++i){
//         printf("Arg %d: %s\n", i, argv[i]);
//     }
// }

// 0 has_mine
// 1 cleared
// 2 flag
// 3 adjacent_mines

// struct Location{
//     int hasMine, cleared, flag, adjacentMines;
// };

/** SECTION: THIS TRANSLATION */
static unsigned int width;
static unsigned int height;
// static struct Location *vboard;
static unsigned char *vboard;

const static int neighbour_map[] = {
    0, 1, 1, 1, 0, -1, -1, -1
};

#define bool int
#define true 1
#define false 0

#define MINE 0b10000000
#define CLEAR 0b01000000
#define REF 0b00100000 /* ADJACENT KNOWN */
#define EDGE 0b00010000 /* UNKNKOWN BY KNOWN */
// #define SPACE(x, y) *(vboard+x*width+y)
#define SPACE(x, y) *(vboard+x*height+y)
#define UNSET &= ~
#define SET |=
#define HAS_MINE >>7
#define IS_CLEAR >>6&1
#define IS_REF >>5&1
#define IS_EDGE >>4&1
#define ADJACENT &15

#define NEIGHBOR_X(x, i) x + neighbour_map[i]
#define NEIGHBOR_Y(y, i) y + neighbour_map[(i + 2) % 8]
#define IS_LOCATION(x, y) x >= 0 && y >= 0 && x < width && y < height
/**cursed macro */
#define FOR_EACH_NEIGHBOR(nx, ny, x, y) for(int i=0; i<8 && (nx = NEIGHBOR_X(x, i))*0+1 && (ny = NEIGHBOR_Y(y, i))*0+1; ++i) if(IS_LOCATION(nx, ny)) 

void clear_recursive(int x, int y){
    if(SPACE(x, y) HAS_MINE) return;
    SPACE(x, y) SET CLEAR;
    SPACE(x, y) UNSET EDGE;
    int nx, ny;
    if((SPACE(x, y) ADJACENT) == 0){ // Empty
        FOR_EACH_NEIGHBOR(nx, ny, x, y){
            if(!(SPACE(nx,ny) IS_CLEAR)) clear_recursive(nx, ny);
        }
    }
    else{ // Is a seen REF
        SPACE(x,y) SET REF;
        FOR_EACH_NEIGHBOR(nx, ny, x, y){ // Unknowns are edges
            if(!(SPACE(nx,ny) IS_CLEAR) && !(SPACE(nx,ny) HAS_MINE)) SPACE(nx,ny) SET EDGE;
        }
    }
}

/** "FORCED" 1 mine 1 unknown, etc */
int found;
// int found_list[10][2];
int force(int x, int y){
    // for(int i=0; i<found; ++i){
    //     if(found_list[i][0] == x && found_list[i][1] == y){
    //         printf("Assertion checked twice\n");
    //         return -1;
    //     }
    // }
    // SPACE(x, y) UNSET REF;
    // SPACE(x, y) &= 0b11110000;
    int nx, ny;
    FOR_EACH_NEIGHBOR(nx, ny, x, y){
        if((SPACE(nx,ny) HAS_MINE) && (SPACE(nx,ny) IS_EDGE)){ // is mine
            // for(int i=0; i<found; ++i){
            //     if(found_list[i][0] == nx && found_list[i][1] == ny){
            //         // printf("r: %d,%d\n", nx, ny);
            //         printf("ASSERTION FAILED! MINE SEARCHED TWICE!\n");
            //         return -1;
            //         // continue;
            //     }
            // }
            printf("[%d,%d] mine found\n", nx, ny);
            // found_list[found][0] = nx;
            // found_list[found][1] = ny;
            ++found;
            SPACE(nx,ny) UNSET EDGE;
            int nnx, nny;
            FOR_EACH_NEIGHBOR(nnx, nny, nx, ny){
                // printf("[%d,%d] is neighbor of mine\n", nnx, nny);
                if(!(SPACE(nnx, nny) HAS_MINE)){
                    if((SPACE(nnx, nny) ADJACENT) == 1){
                        // printf("REMOVED REF: [%d, %d]\n",nnx, nny);
                        // SPACE(nnx, nny) UNSET REF;
                        --SPACE(nnx, nny);
                        if(SPACE(nnx, nny) IS_REF) clear_recursive(nnx, nny);
                        SPACE(nnx, nny) UNSET REF;
                        // clear_recursive(nnx, nny);
                    }else{
                        --SPACE(nnx, nny); // --adjacent
                    }
                }
            }
            // SPACE(nx,ny) UNSET EDGE;
        }
    }
    return 0;
}

void solvable(unsigned int swidth, unsigned int sheight, int count, ...){
    printf("Hello from C!\n");
    
    // Assign arguments
    va_list vlst;
    va_start(vlst, count);

    width = swidth;
    height = sheight;
    // struct Location (*board)[width][height];
    unsigned char *board;
    int *sx, *sy, *n_mines;
    board = va_arg(vlst, unsigned char*);
    sx = va_arg(vlst, int*);
    sy = va_arg(vlst, int*);
    n_mines = va_arg(vlst, int*);

    va_end(vlst);

    vboard = malloc(width*height);
    memcpy(vboard, board, width*height);

    found = 0;

    // CHECK FOR SOLVABLILITY
    
    // clear first
    // clear recursive / build ref/edge list
    // do while flag
    //  search ref list / clear & flip flag
    //  if !flag
    //   while any edge unchecked (max is unchecked * 2)
    //    simulate edge state flagged
    //    if cannot be flagged
    //     clear, break, flip flag
    //    if cannot be cleared
    //     flag space, update refs, flip flag
    //   if !flag
    //    move mine if possible, flip flag, break

    // Mines are edges
    for(int x=0; x<width; ++x){
        for(int y=0; y<height; ++y){
            if(SPACE(x, y) HAS_MINE){
                SPACE(x, y) SET EDGE;
                SPACE(x, y) &= 0b11110000;
            }
        }
    }

    clear_recursive(*sx, *sy);
    
    bool stuck = false;
    while(!stuck){
        stuck = true;
        for(int x=0; x<width; ++x){
            for(int y=0; y<height; ++y){
                if((SPACE(x, y) IS_REF)){
                    int unknowns = 0;
                    int nx, ny;
                    FOR_EACH_NEIGHBOR(nx, ny, x, y){
                        if((SPACE(nx, ny) IS_EDGE)) ++unknowns;
                    }
                    if(unknowns == (SPACE(x, y) ADJACENT)){ // 1 edge, 1 adjacent etc
                        printf("solvable: [%d, %d]\n", x, y);
                        if(force(x, y) == 0) stuck = false;
                        // clear_recursive(x, y);
                        // stuck = false;
                    }
                }
            }
        }
        printf("-----\n");
        // if(found > 8) {printf("Broke on many\n");break;}
        // stuck = true;
    }

    /** DEBUG: test macros */
    // SPACE(*sx, *sy) SET MINE;
    // if(SPACE(*sx, *sy) HAS_MINE) printf("POG\n");
    // printf("Adjacent to 0,0: %d\n", SPACE(0, 0) ADJACENT);
    /** DEBUG: print info */
    printf("FOUND: %d\n", found);
    for(int w=0; w<width; ++w){
        printf("\n[%d] ", w);
        for(int h=0; h<height; ++h){
            if((SPACE(w,h) IS_REF) && (SPACE(w,h) IS_EDGE)) printf("[%d,%d] EDGES AND REFS ARE EXCLUSIVE!\n", w, h);
            printf("[%d] m%d c%d a%d e%d r%d ", h, SPACE(w,h) HAS_MINE, SPACE(w,h) IS_CLEAR, SPACE(w,h) ADJACENT, SPACE(w,h) IS_EDGE, SPACE(w,h) IS_REF);
            // if(SPACE(w,h) IS_REF) printf("%d,%d is a ref\n", w, h);
            // if(SPACE(w,h) IS_EDGE) printf("%d,%d is an edge\n", w, h);
            // if(SPACE(w,h) IS_CLEAR) printf("%d,%d is CLEAR\n", w, h);
            // if(SPACE(w,h) HAS_MINE) printf("MINE at %d,%d\n", w, h);
            // printf("[%d,%d] m:%d c:%d a:%d\n",w,h, HAS_MINE(w, h), CLEARED(w, h), ADJACENT_MINES(w, h));
            // board[w*width+h] SET MINE;
        }
    }
    free(vboard);
}