/** solvable.c
 *  
 *  Contains function 'solvable' used in minefield.vala
 * */

/** TODO: Translate to vala */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>


#define bool int
#define true 1
#define false 0

/** SECTION: THIS TRANSLATION */
static unsigned int width;
static unsigned int height;
static unsigned char *vboard;
static int found;

const static int neighbour_map[] = {
    0, 1, 1, 1, 0, -1, -1, -1
};

// const static enum perhaps{
//     CLEAR, MINE
// };

enum edge_possible{
    UNKNOWN = -1, EITHER = 0, _CLEAR, _MINE
};

struct position{
    int x, y;
};

struct edge_knowledge_ref{
    struct edge_knowledge_ref *prev;
    struct position pos; // DEBUG var
    int refCount;
    struct position refs[8];
    enum edge_possible possible; // Known possiblility
    bool maybeMine;                  // Test case
    // bool triedMine;
    struct edge_knowledge_ref *next;
};

/** SECTION: Macros */

#define MINE 0b10000000
#define CLEAR 0b01000000
#define REF 0b00100000 /* VISIBLE NUMBER NEXT TO UNKNOWN SPACE */
#define EDGE 0b00010000 /* UNKNOWN SPACE NEXT TO A REF */
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
#define FOR_EACH_NEIGHBOR(nx, ny, x, y) for(int NO_SHADOW_I=0; NO_SHADOW_I<8 && (nx = NEIGHBOR_X(x, NO_SHADOW_I))|1 && (ny = NEIGHBOR_Y(y, NO_SHADOW_I))|1; ++NO_SHADOW_I) if(IS_LOCATION(nx, ny)) 

static void clear_recursive(int x, int y){
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
static void force(int x, int y){
    int nx, ny;
    FOR_EACH_NEIGHBOR(nx, ny, x, y){
        if((SPACE(nx,ny) HAS_MINE) && (SPACE(nx,ny) IS_EDGE)){ // is mine
            /** DEBUG: Print mine found */
            printf("[%d,%d] mine found\n", nx, ny);
            ++found;
            SPACE(nx,ny) UNSET EDGE;
            int nnx, nny;
            FOR_EACH_NEIGHBOR(nnx, nny, nx, ny){
                // printf("[%d,%d] is neighbor of mine\n", nnx, nny);
                if(!(SPACE(nnx, nny) HAS_MINE)){
                    if((SPACE(nnx, nny) ADJACENT) == 1){
                        --SPACE(nnx, nny);
                        if(SPACE(nnx, nny) IS_REF) clear_recursive(nnx, nny);
                        SPACE(nnx, nny) UNSET REF;
                    }else{
                        --SPACE(nnx, nny); // --adjacent
                    }
                }
            }
        }
    }
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

    // Mines are edges at start due:
    //  Found mines are neither cleared nor an edge
    for(int x=0; x<width; ++x){
        for(int y=0; y<height; ++y){
            if(SPACE(x, y) HAS_MINE){
                SPACE(x, y) SET EDGE;
                SPACE(x, y) &= 0b11110000;
            }
        }
    }

    clear_recursive(*sx, *sy);
    
    bool stuckCounting = false;
    while(1){
        /** Try counting */
        while(!stuckCounting){
            stuckCounting = true;
            for(int x=0; x<width; ++x){
                for(int y=0; y<height; ++y){
                    if((SPACE(x, y) IS_REF)){
                        int unknowns = 0;
                        int nx, ny;
                        FOR_EACH_NEIGHBOR(nx, ny, x, y){
                            if((SPACE(nx, ny) IS_EDGE)) ++unknowns;
                        }
                        if(unknowns == (SPACE(x, y) ADJACENT)){ // 1 edge, 1 adjacent etc
                            // printf("solvable: [%d, %d]\n", x, y);
                            force(x, y);
                            stuckCounting = false;
                        }
                    }
                }
            }
            printf("-----\n");
        }

        if(found == *n_mines) break;
        /** Try not solvable by counting */
        struct edge_knowledge_ref *start = NULL;
        {// Construct nodes
            struct edge_knowledge_ref *last = NULL;
            // struct edge_knowledge_ref empty = {NULL, {{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1}}, UNKNOWN, NULL};
            for(int x=0; x<width; ++x){
                for(int y=0; y<height; ++y){
                    if(SPACE(x, y) IS_EDGE){
                        int nx, ny;
                        { // Make sure edge has reference (possibly wont if mine)
                            bool real = false;
                            FOR_EACH_NEIGHBOR(nx, ny, x, y){
                                if(SPACE(nx, ny) IS_REF) real = true;
                            }
                            if(!real) continue;
                        }
                        struct edge_knowledge_ref *node = (struct edge_knowledge_ref*)malloc(sizeof(struct edge_knowledge_ref));
                        if(!start) start = node;
                        node->prev = last;
                        if(last) node->prev->next = node;
                        node->refCount = 0;
                        FOR_EACH_NEIGHBOR(nx, ny, x, y){
                            if(SPACE(nx, ny) IS_REF){
                                node->refs[node->refCount].x = nx;
                                node->refs[node->refCount].y = ny;
                                ++(node->refCount);
                            }
                        }
                        node->possible = UNKNOWN;
                        node->maybeMine = false;
                        // node->triedMine = false;
                        node->next = NULL;
                        // Debug
                        node->pos.x = x;
                        node->pos.y = y;
                        last = node;
                    }
                }
            }
        }

        /** DEBUG: print edges */
        // for(int w=0; w<width; ++w){
        //     for(int h=0; h<height; ++h){
        //         if(SPACE(w,h) IS_EDGE) printf("%d,%d is an edge\n", w, h);
        //     }
        // }

        struct edge_knowledge_ref *pre = NULL;
        enum edge_possible knownSpace = UNKNOWN;
        struct position knownSpacePos = {0, 0};

        while(knownSpace == UNKNOWN){
            /** INSERT: The big code (Initial possible state) */

            { /** Initial possible state */
                bool debug_status = false;
                struct edge_knowledge_ref *cursor = start;
                int localCount = found;
                if(debug_status) printf("First: [%d, %d]\n", cursor->pos.x, cursor->pos.y);
                while(cursor){
                    bool oversaturated = false;
                    if(!cursor->maybeMine){
                        if(debug_status) printf("[%d, %d] trying clear\n",cursor->pos.x, cursor->pos.y);
                        SPACE(cursor->pos.x, cursor->pos.y) UNSET EDGE;
                        bool hung = false;
                        for(int i=0; i<cursor->refCount; ++i){ // Check left hanging
                            int nx, ny;
                            int adjacentEdges = 0;
                            // if(debug_status) printf(" [%d, %d] checking edges\n",cursor->refs[i].x, cursor->refs[i].y);
                            FOR_EACH_NEIGHBOR(nx, ny, cursor->refs[i].x, cursor->refs[i].y){
                                // if(debug_status) printf(" [%d, %d] check if edge\n",nx, ny);
                                if(SPACE(nx, ny) IS_EDGE){
                                    ++adjacentEdges;
                                    // if(debug_status) printf(" [%d, %d] is edge\n",nx, ny);
                                }else{
                                    // if(debug_status) printf(" [%d, %d] is not an edge\n",nx, ny);
                                }
                            }
                            if((SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT) > adjacentEdges){
                                hung = true;
                                // if(debug_status) printf(" [%d, %d] left hanging %d adjacent, %d edges\n",cursor->refs[i].x, cursor->refs[i].y,(SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT), adjacentEdges);
                            } else{
                                // if(debug_status) printf(" [%d, %d] ok %d adjacent, %d edges\n",cursor->refs[i].x, cursor->refs[i].y,(SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT), adjacentEdges);
                            }
                        }
                        if(hung){ // Continue if none hung
                            if(debug_status) printf("[%d, %d] trying mine\n",cursor->pos.x, cursor->pos.y);
                            // Can't be clear, try mine
                            if(localCount==*n_mines){
                                oversaturated = true;
                                if(debug_status) printf("TOO MANY MINES [%d, %d]\n",cursor->pos.x, cursor->pos.y);
                            }
                            for(int i=0; i<cursor->refCount; ++i){
                                if((SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT) == 0){
                                    oversaturated = true;
                                    if(debug_status) printf("[%d, %d] OVERSATURATED\n",cursor->refs[i].x, cursor->refs[i].y);
                                }
                            } /** TODO: CAUSES ADJACENT CORRUPTION 0 -= 1 */
                            if(oversaturated){
                                // Oversaturated, roll back and place mine at last clear space
                                SPACE(cursor->pos.x, cursor->pos.y) SET EDGE;
                                cursor = cursor->prev;
                                bool rollback_oversaturated;
                                do{
                                    rollback_oversaturated = false;
                                    while(cursor->maybeMine){
                                        if(debug_status) printf("[%d, %d] unset (rollback)\n",cursor->pos.x, cursor->pos.y);
                                        // Set mines back to normal
                                        cursor->maybeMine = false;
                                        --localCount;
                                        SPACE(cursor->pos.x, cursor->pos.y) SET EDGE;
                                        for(int i=0; i<cursor->refCount; ++i){
                                            ++SPACE(cursor->refs[i].x, cursor->refs[i].y);
                                        }
                                        if(!pre && cursor == start){
                                            if(debug_status) printf("[%d, %d] MUST BE MINE (rollback)\n", cursor->pos.x, cursor->pos.y);
                                            knownSpace = _MINE;
                                            knownSpacePos.x = cursor->pos.x;
                                            knownSpacePos.y = cursor->pos.y;
                                            goto DONE;
                                        }
                                        if(cursor == pre){
                                            if(cursor->maybeMine) printf("[%d, %d] MUST BE MINE (rollback, pre)\n", cursor->pos.x, cursor->pos.y);
                                            else printf("[%d, %d] MUST NOT BE MINE (rollback, pre)\n", cursor->pos.x, cursor->pos.y);
                                            // knownSpace = !cursor->maybeMine;
                                            if(!cursor->maybeMine) knownSpace = _CLEAR;
                                            else knownSpace = _MINE;
                                            knownSpacePos.x = cursor->pos.x;
                                            knownSpacePos.y = cursor->pos.y;
                                            goto DONE;
                                        }
                                        cursor = cursor->prev;
                                    }
                                    SPACE(cursor->pos.x, cursor->pos.y) SET EDGE;
                                    // Check if first non-mine can be mine
                                    for(int i=0; i<cursor->refCount; ++i){
                                        if(localCount+1==*n_mines){
                                            oversaturated = true;
                                            if(debug_status) 
                                                printf("TOO MANY MINES [%d, %d] (rollback)\n",cursor->pos.x, cursor->pos.y);
                                        }
                                        if((SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT) == 0){
                                            rollback_oversaturated = true;
                                            if(debug_status) printf("[%d, %d] oversaturated (rollback)\n",cursor->refs[i].x, cursor->refs[i].y);
                                        }
                                    }
                                    // Continue rollback. Space could not be made mine
                                    if(rollback_oversaturated){
                                        cursor = cursor->prev;
                                    }else{ // Set it as mine and advance
                                        if(debug_status) printf("[%d, %d] mine placed (rollback)\n",cursor->pos.x, cursor->pos.y);
                                        SPACE(cursor->pos.x, cursor->pos.y) UNSET EDGE;
                                        cursor->maybeMine = true;
                                        ++localCount;
                                        for(int i=0; i<cursor->refCount; ++i){
                                            --SPACE(cursor->refs[i].x, cursor->refs[i].y);
                                        }
                                        cursor = cursor->next;
                                    }
                                } while(rollback_oversaturated);
                            }else{
                                if(!pre && cursor == start){
                                    if(debug_status) printf("[%d, %d] MUST BE MINE\n", cursor->pos.x, cursor->pos.y);
                                    knownSpace = _MINE;
                                    knownSpacePos.x = cursor->pos.x;
                                    knownSpacePos.y = cursor->pos.y;
                                    goto DONE;
                                }
                                // GTG, place mine
                                ++localCount;
                                cursor->maybeMine = true;
                                for(int i=0; i<cursor->refCount; ++i){
                                    --SPACE(cursor->refs[i].x, cursor->refs[i].y);
                                }
                            }
                        }
                    }else{
                        printf("ASSERTION: maybe mine=false on call\n");
                    }
                    if(!oversaturated) cursor = cursor->next;
                }
                // DONE:
            }
            DONE:

            if(pre && pre->maybeMine){ // Set pre back to normal
                pre->maybeMine = false;
                for(int i=0; i<pre->refCount; ++i){
                    ++SPACE(pre->refs[i].x, pre->refs[i].y);
                }
            }
            if(pre && knownSpace==UNKNOWN){
                pre->possible = EITHER;
            }
            if(pre) start = pre;

            { /** Update knowlege from last and reset board  */
                struct edge_knowledge_ref *cursor = start;
                bool allEither = true;
                while(cursor){
                    switch(cursor->possible){ // update knowledge
                        case UNKNOWN:
                            if(cursor->maybeMine) cursor->possible = _MINE;
                            else cursor->possible = _CLEAR;
                            allEither = false;
                            break;
                        case EITHER: 
                            break;
                        case _CLEAR:
                            // if(cursor->maybeMine) cursor->possible = EITHER;
                            allEither = false;
                            break;
                        case _MINE:
                            // if(!cursor->maybeMine) cursor->possible = EITHER;
                            allEither = false;
                            break;
                    }
                    // reset
                    SPACE(cursor->pos.x, cursor->pos.y) SET EDGE;
                    if(cursor->maybeMine){
                        cursor->maybeMine = false; /** WARN: decrement local count? */
                        for(int i=0; i<cursor->refCount; ++i){
                            ++SPACE(cursor->refs[i].x, cursor->refs[i].y);
                        }
                    }
                    cursor = cursor->next;
                }
                if(allEither){
                    if(knownSpace != UNKNOWN) printf("ASSERTION! allEither and knownSpace!\n");
                    printf("UNSOLVABLE!\n");
                    break;
                }
            }
            if(knownSpace != UNKNOWN){ // Found!
                if(knownSpace == _MINE){ // Find mine, clear recursive
                    printf("Mine AT: [%d, %d]\n", knownSpacePos.x, knownSpacePos.y);
                    SPACE(knownSpacePos.x,knownSpacePos.y) UNSET EDGE;
                    ++found;
                    int nx, ny;
                    FOR_EACH_NEIGHBOR(nx, ny, knownSpacePos.x, knownSpacePos.y){
                        if(!(SPACE(nx, ny) HAS_MINE)){
                            if((SPACE(nx, ny) ADJACENT) == 1){
                                --SPACE(nx, ny);
                                if(SPACE(nx, ny) IS_REF) clear_recursive(nx, ny);
                                SPACE(nx, ny) UNSET REF;
                            }else{
                                --SPACE(nx, ny);
                            }
                        }
                    }
                }else if(knownSpace == _CLEAR){
                    printf("Mine NOT at: [%d, %d]\n", knownSpacePos.x, knownSpacePos.y);
                    clear_recursive(knownSpacePos.x, knownSpacePos.y);
                }else{
                    printf("ASSERTION: known space was not unknown or clear or mine!\n");
                }
            }else{ // Not found. Try again with new pre
                struct edge_knowledge_ref *cursor = start;
                while(1){ // find first not either
                    if(cursor->possible != EITHER){
                        // Set it to opposite and place before start
                        if(cursor->possible == _CLEAR){
                            cursor->maybeMine = true;
                            // ++localCount;
                            cursor->maybeMine = true;
                            for(int i=0; i<cursor->refCount; ++i){
                                --SPACE(cursor->refs[i].x, cursor->refs[i].y);
                            }
                        }
                        else if(cursor->possible == _MINE){
                            cursor->maybeMine = false;
                        }
                        else printf("ASSERTION! first not pre was neither clear nor mine!\n");
                        if(cursor==start){ // Move to before start
                            pre = start;
                            start = start->next;
                            start->prev = pre;
                        }else if(cursor->next == NULL){
                            pre = cursor;
                            cursor->prev->next = NULL;
                            cursor->prev = NULL;
                            cursor->next = start;
                            start->prev = cursor;
                        }else{
                            pre = cursor;
                            cursor->prev->next = cursor->next;
                            cursor->next->prev = cursor->prev;
                            cursor->prev = NULL;
                            cursor->next = start;
                            start->prev = cursor;
                        }
                        break;
                    }
                    cursor = cursor->next;
                }
            }
        }

        // { /** Initial possible state */
        //     bool debug_status = false;
        //     struct edge_knowledge_ref *cursor = start;
        //     int localCount = found;
        //     if(debug_status) printf("First: [%d, %d]\n", cursor->pos.x, cursor->pos.y);
        //     while(cursor){
        //         bool oversaturated = false;
        //         if(!cursor->maybeMine){
        //             if(debug_status) printf("[%d, %d] trying clear\n",cursor->pos.x, cursor->pos.y);
        //             SPACE(cursor->pos.x, cursor->pos.y) UNSET EDGE;
        //             bool hung = false;
        //             for(int i=0; i<cursor->refCount; ++i){ // Check left hanging
        //                 int nx, ny;
        //                 int adjacentEdges = 0;
        //                 // if(debug_status) printf(" [%d, %d] checking edges\n",cursor->refs[i].x, cursor->refs[i].y);
        //                 FOR_EACH_NEIGHBOR(nx, ny, cursor->refs[i].x, cursor->refs[i].y){
        //                     // if(debug_status) printf(" [%d, %d] check if edge\n",nx, ny);
        //                     if(SPACE(nx, ny) IS_EDGE){
        //                         ++adjacentEdges;
        //                         // if(debug_status) printf(" [%d, %d] is edge\n",nx, ny);
        //                     }else{
        //                         // if(debug_status) printf(" [%d, %d] is not an edge\n",nx, ny);
        //                     }
        //                 }
        //                 if((SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT) > adjacentEdges){
        //                     hung = true;
        //                     // if(debug_status) printf(" [%d, %d] left hanging %d adjacent, %d edges\n",cursor->refs[i].x, cursor->refs[i].y,(SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT), adjacentEdges);
        //                 } else{
        //                     // if(debug_status) printf(" [%d, %d] ok %d adjacent, %d edges\n",cursor->refs[i].x, cursor->refs[i].y,(SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT), adjacentEdges);
        //                 }
        //             }
        //             if(hung){ // Continue if none hung
        //                 if(debug_status) printf("[%d, %d] trying mine\n",cursor->pos.x, cursor->pos.y);
        //                 // Can't be clear, try mine
        //                 if(localCount==*n_mines){
        //                     oversaturated = true;
        //                     if(debug_status) printf("TOO MANY MINES [%d, %d]\n",cursor->pos.x, cursor->pos.y);
        //                 }
        //                 for(int i=0; i<cursor->refCount; ++i){
        //                     if((SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT) == 0){
        //                         oversaturated = true;
        //                         if(debug_status) printf("[%d, %d] OVERSATURATED\n",cursor->refs[i].x, cursor->refs[i].y);
        //                     }
        //                 } /** TODO: CAUSES ADJACENT CORRUPTION 0 -= 1 */
        //                 if(oversaturated){
        //                     // Oversaturated, roll back and place mine at last clear space
        //                     SPACE(cursor->pos.x, cursor->pos.y) SET EDGE;
        //                     cursor = cursor->prev;
        //                     bool rollback_oversaturated;
        //                     do{
        //                         rollback_oversaturated = false;
        //                         while(cursor->maybeMine){
        //                             if(debug_status) printf("[%d, %d] unset (rollback)\n",cursor->pos.x, cursor->pos.y);
        //                             // Set mines back to normal
        //                             cursor->maybeMine = false;
        //                             --localCount;
        //                             SPACE(cursor->pos.x, cursor->pos.y) SET EDGE;
        //                             for(int i=0; i<cursor->refCount; ++i){
        //                                 ++SPACE(cursor->refs[i].x, cursor->refs[i].y);
        //                             }
        //                             if(!pre && cursor == start){
        //                                 if(debug_status) printf("[%d, %d] MUST BE MINE (rollback)\n", cursor->pos.x, cursor->pos.y);
        //                                 knownSpace = MINE;
        //                                 knownSpacePos.x = cursor->pos.x;
        //                                 knownSpacePos.y = cursor->pos.y;
        //                                 goto DONE;
        //                             }
        //                             if(cursor == pre){
        //                                 if(debug_status && !cursor->maybeMine) printf("[%d, %d] MUST BE MINE (rollback, pre)\n", cursor->pos.x, cursor->pos.y);
        //                                 if(debug_status && cursor->maybeMine) printf("[%d, %d] MUST NOT BE MINE (rollback, pre)\n", cursor->pos.x, cursor->pos.y);
        //                                 knownSpace = !cursor->maybeMine;
        //                                 knownSpacePos.x = cursor->pos.x;
        //                                 knownSpacePos.y = cursor->pos.y;
        //                                 goto DONE;
        //                             }
        //                             cursor = cursor->prev;
        //                         }
        //                         SPACE(cursor->pos.x, cursor->pos.y) SET EDGE;
        //                         // Check if first non-mine can be mine
        //                         for(int i=0; i<cursor->refCount; ++i){
        //                             if(localCount==*n_mines){
        //                                 oversaturated = true;
        //                                 if(debug_status) printf("TOO MANY MINES [%d, %d] (rollback)\n",cursor->pos.x, cursor->pos.y);
        //                             }
        //                             if((SPACE(cursor->refs[i].x, cursor->refs[i].y) ADJACENT) == 0){
        //                                 rollback_oversaturated = true;
        //                                 if(debug_status) printf("[%d, %d] oversaturated (rollback)\n",cursor->refs[i].x, cursor->refs[i].y);
        //                             }
        //                         }
        //                         // Continue rollback. Space could not be made mine
        //                         if(rollback_oversaturated){
        //                             cursor = cursor->prev;
        //                         }else{ // Set it as mine and advance
        //                             if(debug_status) printf("[%d, %d] mine placed (rollback)\n",cursor->pos.x, cursor->pos.y);
        //                             SPACE(cursor->pos.x, cursor->pos.y) UNSET EDGE;
        //                             cursor->maybeMine = true;
        //                             ++localCount;
        //                             for(int i=0; i<cursor->refCount; ++i){
        //                                 --SPACE(cursor->refs[i].x, cursor->refs[i].y);
        //                             }
        //                             cursor = cursor->next;
        //                         }
        //                     } while(rollback_oversaturated);
        //                 }else{
        //                     if(!pre && cursor == start){
        //                         if(debug_status) printf("[%d, %d] MUST BE MINE\n", cursor->pos.x, cursor->pos.y);
        //                         knownSpace = MINE;
        //                         knownSpacePos.x = cursor->pos.x;
        //                         knownSpacePos.y = cursor->pos.y;
        //                         goto DONE;
        //                     }
        //                     // GTG, place mine
        //                     ++localCount;
        //                     cursor->maybeMine = true;
        //                     for(int i=0; i<cursor->refCount; ++i){
        //                         --SPACE(cursor->refs[i].x, cursor->refs[i].y);
        //                     }
        //                 }
        //             }
        //         }else{
        //             printf("ASSERTION: maybe mine=false on call\n");
        //         }
        //         if(!oversaturated) cursor = cursor->next;
        //     }
        //     DONE:
        // }

        // printf("Possible state found\n");
        { /** DEBUG: Print found state */
            struct edge_knowledge_ref *cursor = start;
            while(cursor){
                if(cursor->maybeMine) printf("Mine could be at [%d,%d]\n", cursor->pos.x, cursor->pos.y);
                cursor = cursor->next;
            }
        }

        {// Delete nodes
            struct edge_knowledge_ref *next;
            while(start){
                next = start->next;
                free(start);
                start = next;
            }
        }
        break;

        // List of edges -> unknown=-1 | either=0 | clear | mine
        // Use recursive decent to find possible state
        // --adjacent on way down, ++adjacent on way up
        // Working list of edges is copied after possible state found
        // WHILE 
        //  Working list is searched for first !either ASSERTION NOT UNKNOWN
        //  repeat recursive decent while conforming to edge knowledge
        //  IF ALL == Either, unsolvable
        //   fix unsolvable
    }

    /** DEBUG: test macros */
    // SPACE(*sx, *sy) SET MINE;
    // if(SPACE(*sx, *sy) HAS_MINE) printf("POG\n");
    // printf("Adjacent to 0,0: %d\n", SPACE(0, 0) ADJACENT);
    /** DEBUG: print info */
    bool debug = true;
    if(debug) printf("FOUND: %d\n", found);
    for(int w=0; w<width; ++w){
        if(debug) printf("\n[%d] ", w);
        for(int h=0; h<height; ++h){
            if((SPACE(w,h) IS_REF) && (SPACE(w,h) IS_EDGE)) printf("[%d,%d] EDGES AND REFS ARE EXCLUSIVE!\n", w, h);
            if(debug) printf("[%d] m%d c%d a%d e%d r%d ", h, SPACE(w,h) HAS_MINE, SPACE(w,h) IS_CLEAR, SPACE(w,h) ADJACENT, SPACE(w,h) IS_EDGE, SPACE(w,h) IS_REF);
            // if(SPACE(w,h) IS_REF) printf("%d,%d is a ref\n", w, h);
            // if(SPACE(w,h) IS_EDGE) printf("%d,%d is an edge\n", w, h);
            // if(SPACE(w,h) IS_CLEAR) printf("%d,%d is CLEAR\n", w, h);
            // if(SPACE(w,h) HAS_MINE) printf("MINE at %d,%d\n", w, h);
        }
    }
    if(debug) printf("\n");
    free(vboard);
}