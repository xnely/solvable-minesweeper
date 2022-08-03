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
            // printf("[%d,%d] mine found\n", nx, ny);
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
    // if(!force_success) {printf("ASSERTION! FORCE MUST ALWAYS SUCCEED!\n"); return 1;}
}

/** TODO: Case not handled
 * 
 *  When no unseen clear spaces are available,
 *  states where not all mines are used are impossible.
 * 
 *  Reaching end of edge list without all mines (if above is true)
 *  should cause a rollback
 * */

int solvable(unsigned int swidth, unsigned int sheight, int count, ...){
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

    // Used for managing moving mines
    bool move_success = true;
    bool move_active = false;
    struct pos_list{
        struct position pos;
        struct pos_list *next;
    };
    struct pos_list *available_spaces = NULL;

    bool solvable = true;
    while(solvable){
        /** Try counting */
        bool stuckCounting = false;
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
                            if(move_active){
                                move_success = true;
                                struct pos_list *cursor = available_spaces;
                                while(cursor){
                                    struct pos_list *tmp = cursor;
                                    cursor = cursor->next;
                                    free(tmp);
                                }
                                available_spaces = NULL;
                                move_active = false;
                            }
                            stuckCounting = false;
                        }
                    }
                }
            }
            // printf("-----\n");
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
        int clear_spaces = 0;

        int min_mines = INT_MAX;
        while(knownSpace == UNKNOWN){
            int localCount = found;
            if(pre && pre->maybeMine) ++localCount;
            /** Find possible state */
            {
                bool debug_status = false;
                struct edge_knowledge_ref *cursor = start;
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
                            }
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
                                            if(debug_status && cursor->maybeMine) printf("[%d, %d] MUST BE MINE (rollback, pre)\n", cursor->pos.x, cursor->pos.y);
                                            else if(debug_status) printf("[%d, %d] MUST NOT BE MINE (rollback, pre)\n", cursor->pos.x, cursor->pos.y);
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
                                        if(localCount==*n_mines){
                                            oversaturated = true;
                                            if(debug_status) printf("TOO MANY MINES [%d, %d] (rollback)\n",cursor->pos.x, cursor->pos.y);
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
                SPACE(pre->pos.x, pre->pos.y) SET EDGE; /** DEBUG: pre was not edge! */
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
                /** Check for unseen */
                if(localCount < min_mines) min_mines = localCount;
                if(allEither){
                    if(knownSpace != UNKNOWN) printf("ASSERTION! allEither and knownSpace!\n");
                    bool found_unseen = false;
                    if(*n_mines - min_mines == 0){
                        for(int x=0; x<width; ++x){
                            for(int y=0; y<height; ++y){
                                if(!(SPACE(x, y) IS_CLEAR) && !(SPACE(x, y) IS_EDGE) && !(SPACE(x, y) HAS_MINE)){
                                    found_unseen = true;
                                    // printf("CLEARED [%d, %d] BY MINE COUNT!!!! ---------------------------------------\n", x, y);
                                    clear_recursive(x, y);
                                    // Hack to pass
                                    knownSpace = _CLEAR;
                                    knownSpacePos.x = x;
                                    knownSpacePos.y = y;
                                }
                            }
                        }
                    }
                    if(!found_unseen){
                        solvable = false;
                        knownSpace = CLEAR;
                    }
                    
                }
                if(knownSpace != UNKNOWN && solvable){ // Found!
                    if(move_active){
                        move_success = true;
                        struct pos_list *cursor = available_spaces;
                        while(cursor){
                            struct pos_list *tmp = cursor;
                            cursor = cursor->next;
                            free(tmp);
                        }
                        available_spaces = NULL;
                        move_active = false;
                    }
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
                }else if(solvable){ // Not found. Try again with new pre
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
                            SPACE(pre->pos.x, pre->pos.y) UNSET EDGE; /** DEBUG: pre not edge! */
                            break;
                        }
                        cursor = cursor->next;
                    }
                }
            }
        }

        // printf("Possible state found\n");
        // { /** DEBUG: Print found state */
        //     struct edge_knowledge_ref *cursor = start;
        //     while(cursor){
        //         if(cursor->maybeMine) printf("Mine could be at [%d,%d]\n", cursor->pos.x, cursor->pos.y);
        //         cursor = cursor->next;
        //     }
        // }

        {// Delete nodes
            struct edge_knowledge_ref *next;
            while(start){
                next = start->next;
                free(start);
                start = next;
            }
        }
        if(!solvable){
            printf("Unsolvable reached! Trying to move mine...\n");
            move_active = true;
            solvable = true;
            if(move_success){
                move_success = false;
                struct pos_list *available_space = NULL;
                struct pos_list *current = NULL;
                printf("No list yet built.\n");
                clear_spaces = 0;
                // bool used_edge = false;
                for(int x=0; x<width; ++x){ // build list
                    for(int y=0; y<height; ++y){
                        // if(!used_edge){
                        //     bool realEdge = false;
                        //     if(SPACE(x, y) IS_EDGE){
                        //         int nx, ny;
                        //         FOR_EACH_NEIGHBOR(nx, ny, x, y){
                        //             if((SPACE(nx, ny) ADJACENT) > 0){ // WARN: SHOULD BE IS_REF???
                        //                 realEdge = true;
                        //             }
                        //         }
                        //         if(realEdge){
                        //             used_edge = true;
                        //             ++clear_spaces;
                        //             available_space = malloc(sizeof(struct pos_list));
                        //             available_space->next = NULL;
                        //             available_space->pos.x = x;
                        //             available_space->pos.y = y;
                        //             if(!available_spaces){
                        //                 available_spaces = available_space;
                        //             }else{
                        //                 current->next = available_space;
                        //             }
                        //             current = available_space;
                        //         }
                        //     }
                        // }else 
                              if((SPACE(x, y) IS_EDGE) || (!(SPACE(x, y) IS_CLEAR) && !(SPACE(x, y) HAS_MINE))){
                            ++clear_spaces;
                            available_space = malloc(sizeof(struct pos_list));
                            available_space->next = NULL;
                            available_space->pos.x = x;
                            available_space->pos.y = y;
                            if(!available_spaces){
                                available_spaces = available_space;
                            }else{
                                current->next = available_space;
                            }
                            current = available_space;
                        }
                    }
                }
                printf("Spaces to use: %d\n", clear_spaces);
                if(!available_spaces || !available_spaces->next){
                    printf("Not enough spaces available!\n");
                    solvable = false;
                    break;
                }
            }

            struct pos_list *mine_space = NULL;
            struct pos_list *empty_space = NULL;
            {
                struct pos_list *cursor = available_spaces;
                // bool found_mine = false;
                // bool found_empty = false;
                bool used_edge = false;
                while(cursor && (!mine_space || !empty_space)){
                    if(!used_edge){
                        if(SPACE(cursor->pos.x, cursor->pos.y) IS_EDGE){
                            int nx, ny;
                            FOR_EACH_NEIGHBOR(nx, ny, cursor->pos.x, cursor->pos.y){
                                if(!used_edge && SPACE(nx, ny) IS_REF){
                                    used_edge = true;
                                    // printf("[%d, %d] is an edge. using.\n", cursor->pos.x, cursor->pos.y);
                                }
                            }
                        }
                    }
                    if(used_edge){
                        if(!mine_space && SPACE(cursor->pos.x, cursor->pos.y) HAS_MINE){
                            mine_space = cursor;
                        }
                        if(!empty_space && !(SPACE(cursor->pos.x, cursor->pos.y) HAS_MINE)){
                            empty_space = cursor;
                        }
                    }
                    cursor = cursor->next;
                }
                if(!used_edge){
                    printf("No edges are available to move!\n");
                    solvable = false;
                    break;
                }
                if(!mine_space){
                    printf("No mines are available to move!\n");
                    solvable = false;
                    break;
                }
                if(!empty_space){
                    printf("No empty spaces are available to move!\n");
                    solvable = false;
                    break;
                }
            }
            // MOVE Mine
            {
                int nx, ny;
                // Vboard and Real board set mine
                printf("Placing mine at [%d, %d]\n", empty_space->pos.x, empty_space->pos.y);
                // SPACE(empty_space->pos.x, empty_space->pos.y) UNSET REF;
                SPACE(empty_space->pos.x, empty_space->pos.y) SET EDGE;
                SPACE(empty_space->pos.x, empty_space->pos.y) SET MINE;
                ++SPACE(empty_space->pos.x, empty_space->pos.y); /** DEBUG: Set mine needs to have the right mine count */
                *(board+empty_space->pos.x*height+empty_space->pos.y) SET MINE;
                FOR_EACH_NEIGHBOR(nx, ny, empty_space->pos.x, empty_space->pos.y){
                    ++SPACE(nx, ny);
                    ++(*(board+nx*height+ny));
                }
                // Vboard and Real board unset mine
                printf("Removing mine at [%d, %d]\n", mine_space->pos.x, mine_space->pos.y);
                SPACE(mine_space->pos.x, mine_space->pos.y) UNSET MINE;

                printf("bf: Adjacent of [%d, %d]: %d\n", mine_space->pos.x, mine_space->pos.y, (SPACE(mine_space->pos.x, mine_space->pos.y) ADJACENT));
                {
                    int adjacent = *(board+mine_space->pos.x*height+mine_space->pos.y) ADJACENT;
                    printf("bf: real adjacent: %d\n", adjacent);
                    FOR_EACH_NEIGHBOR(nx, ny, mine_space->pos.x, mine_space->pos.y){
                        if(SPACE(nx, ny) HAS_MINE && !(SPACE(nx, ny) IS_EDGE)){
                            printf("bf: not counting known mine as adjacent\n");
                            --adjacent;
                        }
                    }
                    SPACE(mine_space->pos.x, mine_space->pos.y) &= 0b11110000;
                    SPACE(mine_space->pos.x, mine_space->pos.y) |= adjacent;
                }
                printf("aft: Adjacent of [%d, %d]: %d\n", mine_space->pos.x, mine_space->pos.y, (SPACE(mine_space->pos.x, mine_space->pos.y) ADJACENT));
                *(board+mine_space->pos.x*height+mine_space->pos.y) UNSET MINE;
                FOR_EACH_NEIGHBOR(nx, ny, mine_space->pos.x, mine_space->pos.y){
                    if(((SPACE(nx, ny) ADJACENT) == 0) && !(SPACE(nx, ny) HAS_MINE))
                        printf("Why does a space next to an edge mine have 0 ADJACENT????\n");
                    if(!((SPACE(nx, ny) ADJACENT) == 0))
                        --SPACE(nx, ny);
                    --(*(board+nx*height+ny));
                    if((SPACE(nx, ny) ADJACENT) == 0 && !(SPACE(nx, ny) HAS_MINE) && (SPACE(nx, ny) IS_CLEAR)){ // Don't clear unseen
                        printf("Clearing space [%d, %d]\n", nx, ny);
                        SPACE(nx, ny) UNSET REF; // Cleared space should not be a ref
                        clear_recursive(nx, ny);
                    } else if((SPACE(nx, ny) ADJACENT) == 0 && !(SPACE(nx, ny) HAS_MINE) && (SPACE(nx, ny) IS_REF) && !(SPACE(nx, ny) IS_CLEAR)){
                        printf("Unset REF for apparently unseen space? AT [%d, %d]\n", nx, ny);
                        SPACE(nx, ny) UNSET REF;
                    }
                }
                // Remove empty_space from list
                if(mine_space == available_spaces){
                    available_spaces = available_spaces->next;
                    free(mine_space);
                    mine_space = NULL;
                }
                struct pos_list *cursor = available_spaces;
                while(cursor){
                    if(mine_space && cursor->next == mine_space){
                        cursor->next = mine_space->next;
                        free(mine_space);
                        mine_space = NULL;
                        break;
                    }
                    cursor = cursor->next;
                }
                knownSpace = CLEAR;
            }
        }
    }

    /** One last test if unseen spaces are all mines */
    if(found != *n_mines){
        int unfound = *n_mines - found;
        int unseen = 0;
        for(int x=0; x<width; ++x){
            for(int y=0; y<height; ++y){
                if(SPACE(x, y) IS_EDGE) goto DONE_ALL_MINES_TEST;
                if(!(SPACE(x, y) IS_CLEAR) && !(SPACE(x, y) IS_EDGE) && !(SPACE(x, y) HAS_MINE)) ++unseen;
            }
        }
        if(unseen == unfound){
            found = *n_mines;
            printf("Final found by mine count");
        }
        DONE_ALL_MINES_TEST:;
    }

    /** DEBUG: test macros */
    // SPACE(*sx, *sy) SET MINE;
    // if(SPACE(*sx, *sy) HAS_MINE) printf("POG\n");
    // printf("Adjacent to 0,0: %d\n", SPACE(0, 0) ADJACENT);
    /** DEBUG: print info */
    bool debug = false;
    printf("FOUND: %d\n", found);
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
    
    if(found == *n_mines) return 0;
    else return -1;
}