//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

static game_server *SERVER;
struct game *GAME;
int playerCount = 0;
bool game_begin = false;

void init_server() {
    if (SERVER == NULL) {
        SERVER = calloc(1, sizeof(struct game_server));
    } else {
        printf("Server already started");
    }
}

int handle_client_connect(int player) {
    // STEP 8 - This is the big one: you will need to re-implement the REPL code from
    // the repl.c file, but with a twist: you need to make sure that a player only
    // fires when the game is initialized and it is there turn.  They can broadcast
    // a message whenever, but they can't just shoot unless it is their turn.
    //
    // The commands will obviously not take a player argument, as with the system level
    // REPL, but they will be similar: load, fire, etc.
    //
    // You must broadcast informative messages after each shot (HIT! or MISS!) and let
    // the player print out their current board state at any time.
    //
    // This function will end up looking a lot like repl_execute_command, except you will
    // be working against network sockets rather than standard out, and you will need
    // to coordinate turns via the game::status field.

    int client_sock = SERVER->player_sockets[player];

    int read_size;
    char_buff* output = cb_create(2000);
    char_buff* input = cb_create(2000);
    char_buff* inputChat = cb_create(2000);
    char raw_buffer[2000];
    GAME = game_get_current();

    cb_append(output, "Welcome to Battlebit server Player ");
    cb_append_int(output, player);
    cb_append(output, "\n");
    cb_append(output, "battleBit (? for help) > ");
    cb_write(client_sock, output);
    cb_reset(output);

    cb_append(inputChat, "Player ");
    cb_append_int(inputChat, player);
    cb_append(inputChat, " said: ");


    while ((read_size = recv(client_sock, raw_buffer, 2000, 0)) > 0){
        raw_buffer[read_size] = '\0';
        cb_reset(input);
        cb_append(input, raw_buffer);

        char* command = cb_tokenize(input, " ");
        if (command) {
            char* arg1 = cb_next_token(input);
            char* arg2 = cb_next_token(input);
            if (strcmp(command, "exit\r\n") == 0) {
                cb_append(output, "goodbye!\n");
                cb_write(client_sock, output);
                exit(EXIT_SUCCESS);
            } else if(strcmp(command, "?\r\n") == 0) {

                cb_append(output, "? - show help\n");
                cb_append(output, "load [0-1] <string> - load a ship layout file for the given player\n");
                cb_append(output, "show [0-1] - shows the board for the given player\n");
                cb_append(output, "fire [0-7] [0-7] - fire at the given position\n");
                cb_append(output, "say <string> - Send the string to all players as part of a chat\n");
                cb_append(output, "exit - quit the server\n");
                cb_append(output, "battleBit (? for help) > ");
                cb_write(client_sock, output);

            } else if(strcmp(command, "show\r\n") == 0) {

                struct char_buff *boardBuffer = cb_create(2000);
                repl_print_board(game_get_current(), player, boardBuffer);
                cb_append(output, boardBuffer->buffer);
                cb_free(boardBuffer);
                cb_write(client_sock, output);

            } else if (strcmp(command, "load") == 0) {

                game_load_board(game_get_current(), player, arg1);
                playerCount++;
                if (playerCount == 1){
                    cb_append(output, "Waiting on Player ");
                    cb_append_int(output, (1-player));
                    cb_append(output, "\n");
                    cb_append(output, "battleBit (? for help) > ");
                    cb_write(client_sock, output);
                    cb_reset(output);
                } else if (playerCount == 2){
                    cb_append(output, "\nAll Player Boards Loaded\n");
                    cb_append(output, "Player 0's Turn\n");
                    cb_append(output, "battleBit (? for help) > ");
                    server_broadcast(output);
                    game_begin = true;
                }


            } else if (strcmp(command, "fire") == 0) {
                GAME = game_get_current();
                int x = atoi(arg1);
                int y = atoi(arg2);
                if (game_begin && (player == 0) && (GAME->status == PLAYER_0_TURN)) {
                    if (x < 0 || x >= BOARD_DIMENSION || y < 0 || y >= BOARD_DIMENSION) {
                        // printf("Invalid coordinate: %i %i\n", x, y);
                        cb_append(output, "Invalid Coordinate ");
                        cb_append_int(output, x);
                        cb_append(output, " ");
                        cb_append_int(output, y);
                    } else {
                        // printf("Player %i fired at %i %i\n", player + 1, x, y);
                        cb_append(output, "Player ");
                        cb_append_int(output, player);
                        cb_append(output, " fired at ");
                        cb_append_int(output, x);
                        cb_append(output, " ");
                        cb_append_int(output, y);
                        int result = game_fire(game_get_current(), player, x, y);
                        if (result) {
                            cb_append(output, " - HIT!!!\n");
                            cb_append(output, "battleBit (? for help) > ");
                            server_broadcast(output);
                        } else {
                            cb_append(output, " - Miss\n");
                            cb_append(output, "battleBit (? for help) > ");
                            server_broadcast(output);
                        }
                    }

                } else if (game_begin && (player == 1) && (GAME->status == PLAYER_1_TURN)) {
                    if (x < 0 || x >= BOARD_DIMENSION || y < 0 || y >= BOARD_DIMENSION) {
                        // printf("Invalid coordinate: %i %i\n", x, y);
                        cb_append(output, "Invalid Coordinate ");
                        cb_append_int(output, x);
                        cb_append(output, " ");
                        cb_append_int(output, y);
                    } else {
                        // printf("Player %i fired at %i %i\n", player + 1, x, y);
                        cb_append(output, "Player ");
                        cb_append_int(output, player);
                        cb_append(output, " fired at ");
                        cb_append_int(output, x);
                        cb_append(output, " ");
                        cb_append_int(output, y);
                        int result = game_fire(game_get_current(), player, x, y);
                        if (result) {
                            cb_append(output, " - HIT!!!\n");
                            cb_append(output, "battleBit (? for help) > ");
                            server_broadcast(output);
                        } else {
                            cb_append(output, " - Miss\n");
                            cb_append(output, "battleBit (? for help) > ");
                            server_broadcast(output);
                        }
                    }

                } else if (game_begin){
                    cb_append(output, "\nIt is not your turn.\n");
                    cb_append(output, "battleBit (? for help) > ");
                    cb_write(client_sock, output);
                } else {
                    cb_append(output, "\nGame has not begun.\n");
                    cb_append(output, "battleBit (? for help) > ");
                    cb_write(client_sock, output);
                }

            } else if(strcmp(command, "say") == 0){
                cb_append(inputChat, raw_buffer + 4);
                cb_append(inputChat, "battleBit (? for help) > ");
                server_broadcast(inputChat);
                cb_reset(inputChat);



            } else {
                cb_append(output, "Unknown Command\n");
            }
        }

        cb_reset(output);

    }

}

void server_broadcast(char_buff *msg) {
    // send message to all players
    for (int i = 0; i < 2; i++) {
        int client_sock = SERVER->player_sockets[i];
        cb_write(client_sock, msg);
    }


}

int run_server() {
    // STEP 7 - implement the server code to put this on the network.
    // Here you will need to initalize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously
    int server_socket_fd = socket(AF_INET,
                                  SOCK_STREAM,
                                  IPPROTO_TCP);
    if (server_socket_fd == -1) {
        printf("Could not create socket\n");
    }

    int yes = 1;
    setsockopt(server_socket_fd,
               SOL_SOCKET,
               SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server;

    // fill out the socket information
    server.sin_family = AF_INET;
    // bind the socket on all available interfaces
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9876);

    int request = 0;
    if (bind(server_socket_fd,
            // Again with the cast
             (struct sockaddr *) &server,
             sizeof(server)) < 0) {
        puts("Bind failed");
    } else {
        puts("Bind worked!");
        listen(server_socket_fd, 3);

        //Accept an incoming connection
        puts("Waiting for incoming connections...");


        struct sockaddr_in client;
        socklen_t size_from_connect;
        int client_socket_fd;
        int player = 0;
        while ((client_socket_fd = accept(server_socket_fd,
                                          (struct sockaddr *) &client,
                                          &size_from_connect)) > 0) {
            /*char message[100] = {0};
            sprintf(message,
                    "Thank you for coming, come again - req %d\n\n",
                    request_count++);*/

            SERVER->player_sockets[player] = client_socket_fd;
            pthread_t playerThread;
            pthread_create(&playerThread, NULL, handle_client_connect, player);
            SERVER->player_threads[player] = playerThread;
            player++;

            /*send(client_socket_fd, message,
                 strlen(message), 0);*/
            //close(client_socket_fd);
        }
    }
}

int server_start() {
    // STEP 6 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL
    init_server();
    pthread_create(&SERVER->server_thread, NULL, (void *) run_server, NULL);
}
