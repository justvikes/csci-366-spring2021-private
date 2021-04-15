//
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <stdio.h>
#include "game.h"
// STEP 9 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server. Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.

static game * GAME = NULL;

void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int game_fire(game *game, int player, int x, int y) {
    // Step 5 - This is the crux of the game. You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    // - You will need up update the players 'shots' value
    // - you You will need to see if the shot hits a ship in the opponents ships value. If so, record a hit in the
    // current players hits field
    // - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    // If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    // PLAYER_1_WINS or PLAYER_2_WINS depending on who won.
    player_info *shooter_info = &game->players[player];

    //player getting hit
    player_info *shootee_info = &game->players[1-player];

    //mask of shot from shooter
    unsigned long long int bitmask = xy_to_bitval(x, y);

    //update shots for shooter
    shooter_info->shots = shooter_info->shots | bitmask;

    if(game->status == PLAYER_0_TURN){
        game->status = PLAYER_1_TURN;
    }
    else if(game->status == PLAYER_1_TURN){
        game->status = PLAYER_0_TURN;
    }
    //updating if there are hits
    if((shootee_info->ships & bitmask) != 0ULL){
        //hurt_info->ships &= ~(1UL << mask);
        //shooter_info->hits |= 1UL << mask;
        shootee_info->ships = shootee_info->ships & (~bitmask);
        shooter_info->hits = shooter_info->hits | bitmask;
        if(shootee_info->ships == 0ULL) {
            if(player == 1){
                game->status = PLAYER_1_WINS;
            }
            else{
                game->status = PLAYER_0_WINS;
            }
        }
        return 1;
    }

    return 0;
}

unsigned long long int xy_to_bitval(int x, int y) {
    // Step 1 - implement this function. We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b00000...0001 (the one is in the first position)
    // x:1, y: 0 == 0b00000...10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.
    unsigned long long int bitval = 0ULL;
    if(x<0 | x>7 | y<0 | y> 7){
        return bitval;
    }
    else{
        bitval |= 1UL << (x + (8*y));
        return bitval;
    }

}

struct game * game_get_current() {
    return GAME;
}

int game_load_board(struct game *game, int player, char * spec) {
    // Step 2 - implement this function. Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //

    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //
    // if it is invalid, you should return -1
    
    //number of characters in the string
    int specPos = 0;
    // ship index is the position of the ship type in the spec
    int shipIndex = 0;

    player_info *player_info = &game->players[player];

    if(spec == NULL ){
        return -1;
    }
    for(int i=0; i < 5; i++){
        if(spec[shipIndex] == '\0' | spec[shipIndex+1] == '\0' | spec[shipIndex+2] == '\0'){
            return -1;
        }
        int ship_size;
        ship_size = shipLength(spec[shipIndex]);
        if(ship_size == 0){
            return -1;
        }
        else{
            //here we will test add the ships
            //horizontal add
            if(spec[shipIndex] >= 'A' & spec[shipIndex] <= 'Z'){
                int test;
                test = add_ship_horizontal(player_info, (spec[shipIndex + 1] - '0'), (spec[shipIndex + 2] - '0'),
                                           ship_size);
                if(test == -1){
                    return -1;
                }

            }
                //vertical add
            else if(spec[shipIndex] >= 'a' & spec[shipIndex] <= 'z'){
                int test;
                test = add_ship_vertical(player_info, (spec[shipIndex + 1] - '0'), (spec[shipIndex + 2] - '0'), ship_size);
                if(test == -1){
                    return -1;
                }
            }
        }
        shipIndex += 3;
        specPos++;

    }
    int turnCount = 0;
    if(specPos != 5){
        return -1;
    }
    else{
        for(int i = 0; i < 2; i++){
            if(game->players[i].ships != 0){
                turnCount++;
            }
        }
        if(turnCount == 2){
            game->status = PLAYER_0_TURN;
        }
        return 1;
    }
}


int shipLength(char x) {
    if(x == 'C' | x == 'c'){
        return 5;
    }
    if(x == 'B' | x == 'b'){
        return 4;
    }
    if(x == 'D' | x == 'd'){
        return 3;
    }
    if(x == 'S' | x == 's'){
        return 3;
    }
    if(x == 'P' | x == 'p'){
        return 2;
    }
    else{
        return 0;
    }
}

int add_ship_horizontal(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    if(length == 0){
        return 1;
    }
    else if((player->ships & xy_to_bitval(x, y)) != 0ULL){
        return -1;
    }
    else{
        player->ships = player->ships | xy_to_bitval(x, y);
        return add_ship_horizontal(player, x + 1, y, length - 1);
    }

}

int add_ship_vertical(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    if(length == 0){
        return 1;
    }
    else if((player->ships & xy_to_bitval(x, y)) != 0ULL){
        return -1;
    }
    else{
        player->ships = player->ships | xy_to_bitval(x, y);
        return add_ship_vertical(player, x, y+ 1, length - 1);
    }
}