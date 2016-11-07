#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <error.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "simple_chat.h"

// Prints out a list of available commands
void server_help() {
    printf("\nVChat v1.0 Help\n"
           "\t/help - Prints this help message\n"
           "\t/dc <username> - Disconnects the given user\n"
           "\t/quit - Exits the client\n\n");
    return;
}

// Sends a message to all clients. 
void broadcast(int client_fd, char** client_names, fd_set clients, char* message) {
    int  username_flag = 0;
    char buffer[MAX_MESSAGE_SIZE];
    char bc_message[MAX_SEGMENT_SIZE];
    memset(buffer, 0, sizeof(buffer));
    memset(bc_message, 0, sizeof(buffer));

    if (strstr(message, "%s") != NULL) {
        username_flag = 1;
        strncpy(buffer, message+2, sizeof(buffer) - 4);
    }

    else strncpy(buffer, message, sizeof(buffer) - 2);

    // format username into buffer
    if (username_flag)
        snprintf(bc_message, sizeof(bc_message), "BROADCAST %s%s", client_names[client_fd], buffer);

    else
        snprintf(bc_message, sizeof(bc_message), "BROADCAST %s", buffer);

    // print for server, if not server broadcasting
    if (client_fd != 0) {
        printf("\33[2K\r");
        printf("%s", bc_message+10);
        prompt();
    }

    int i;

    // loop through each client, sending the broadcast
    for (i = 0; i < FD_SETSIZE; i++) {
        // skip if invalid or current client
        if (!FD_ISSET(i, &clients) || i == client_fd)
            continue;

        // error
        if (send_all(i, bc_message)) {
            fprintf(stderr, "\33[2K\r");
            fprintf(stderr, "Error - Broadcast to %u\n", i);
        }
    }

    return;
}

// Disconnects all clients.
void dc_all(char** client_names, fd_set *clients, fd_set *active) {
    int i;

    // loop through each client, sending a goodbye
    for (i = 1; i < FD_SETSIZE; i++) {
        // skip if invalid
        if (!FD_ISSET(i, clients))
            continue;

        // error
        if (send_all(i, "GOODBYE")) {
            fprintf(stderr, "\33[2K\r");
            fprintf(stderr, "Error - GOODBYE to %u\n", i);
        }

        // clean up socket and fd_lists
        close(i);
        FD_CLR(i, active);
        FD_CLR(i, clients);
        client_names[i][0] = 0;
    }

    return;
}

// Interpret user commands
int server_command(char** client_names, fd_set *clients, fd_set *active, char *message) {
    // strip newline character
    message[strlen(message)-1] = 0;

    // if it is a command:
    if (message[0] == '/') {

        // shut down server, dc'ing all clients
        if (strcmp(message+1, "quit") == 0) {
            printf("\33[2K\rBye...\n");
            dc_all(client_names, clients, active);
            exit(0);
        }

        if (strcmp(message+1, "help") == 0) {
            server_help();
            return 0;
        }

        if (strstr(message+1, "dc ") == message+1) {

            // if dc'ing self, treat it as a '/quit'
            if (strcmp(message+4, username) == 0) {
                printf("\33[2K\rBye...\n");
                dc_all(client_names, clients, active);
                exit(0);
            }

            int i;

            // otherwise search for username and send dc
            for (i = 1; i < FD_SETSIZE; i++) {
                if (client_names[i] != NULL && strcmp(message+4, client_names[i]) == 0) {
                    send_all(i, "GOODBYE");

                    // remove from fd_sets and broadcast their departure
                    FD_CLR(i, active);
                    FD_CLR(i, clients);
                    broadcast(i, client_names, *clients, "%s has left the chat.\r\n");
                    
                    // close socket and clear name
                    close(i);
                    client_names[i][0] = 0;
                    return 0;
                }
            }

            // if dc all
            if (strstr(message+1, "dc all") == message+1) {
                dc_all(client_names, clients, active);
                return 0;
            }
        }
    }

    // otherwise its a broadcast
    else {

        // if its was just a newline, treat it as an accident and ignore
        if (message[0] == 0)
            return 1;

        char buffer[MAX_MESSAGE_SIZE];
        memset(buffer, 0, sizeof(buffer));

        snprintf(buffer, sizeof(buffer), "%%s: %s\r\n", message);
        broadcast(0, client_names, *clients, buffer);
        return 0;
    }

    return 0;
}

// Handles an incoming connection. Requires the following message format:
//
//     HELLO I'M <username>
//
// If the client does not follow protocol or provides an invalid username,
// an error message is printed. If a username has already been taken, the
// client is prompted for another one. Returns 0 if client successfully
// connected; 1 otherwise. 
int receive_client(int client_fd, char** client_names) {
    static char message[MAX_SEGMENT_SIZE];
    static char buffer[MAX_SEGMENT_SIZE];
    static int num_tries = 1;
    memset(message, 0, sizeof(message));
    memset(buffer, 0, sizeof(buffer));
    
    // if no valid line ending (malformed message)
    if (recv_all(client_fd, message, sizeof(message))) {
        printf("\33[2K\r");
        fprintf(stdout, "ERROR - Malformed message from fd %u: %s", client_fd, message);
        return 1;
    }

    // Malformed message
    if (strstr(message, "HELLO I'M ") != message) {
        printf("\33[2K\r");
        fprintf(stdout, "ERROR - Malformed message from fd %u: %s", client_fd, message);
        return 1;
    }

    // username starts at message + 10, don't count ending \r\n
    size_t len = strlen(message + 10) - 2;
    
    // Username size check
    if (len >= USERNAME_SIZE) {
        fprintf(stdout, "ERROR - Username from %u too large: %s", client_fd, message+10);
        return 1;
    }

    if (len <= 0) {
        fprintf(stdout, "ERROR - Username from %u not present: %s", client_fd, message+10);
        return 1; 
    }

    // Check if name has been used before
    strncpy(buffer, message+10, len);

    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
        // if name not in use
        if (client_names[i] == NULL)
            continue;

        // if this name has been used already
        if (strcmp(buffer, client_names[i]) == 0) {
            send_all(client_fd, "IN USE");
            
            // If more than 10 tries, disconnect them
            if (++num_tries >= 10) {
                num_tries = 1;
                send_all(client_fd, "GOODBYE");
                return 1;
            }

            // Make recursive call to handle checks
            return receive_client(client_fd, client_names);
        }
    }

    // Successfully connected
    num_tries = 1;
    client_names[client_fd] = realloc(client_names[client_fd], len);

    // error
    if (client_names[client_fd] == NULL) {
        perror("receive_client: realloc");
        return 1;
    }

    strncpy(client_names[client_fd], buffer, len);
    client_names[client_fd][len] = 0;
    send_all(client_fd, "WELCOME!");
    return 0;
}

// TODO: return 0 on success, any other value if client disconnected 
int handle_client(int client_fd, char **client_names, fd_set clients) {
    char message[MAX_SEGMENT_SIZE];
    memset(message, 0, sizeof(message));

    // if no line ending
    if (recv_all(client_fd, message, sizeof(message))) {
        printf("\33[2K\r");
        fprintf(stderr, "ERROR - Malformed message from fd %u: %s", client_fd, message);
        return 1;
    }

    // If client wishes to broadcast a message
    if (strstr(message, "BROADCAST ") == message) {
        char buffer[MAX_MESSAGE_SIZE];
        memset(buffer, 0, sizeof(buffer));

        snprintf(buffer, sizeof(buffer), "%%s: %s", message + 10);
        broadcast(client_fd, client_names, clients, buffer);
        return 0;
    }

    // client disconnected
    else if (strstr(message, "GOODBYE") == message)
        return 1;

    // Malformed message
    else {
        printf("\33[2K\r");
        fprintf(stderr, "ERROR - Malformed message from %s: %s\n", client_names[client_fd], message);
        return 1;
    }
}

void chat_server(int port) {
    char *client_names[FD_SETSIZE];
    memset(client_names, 0, sizeof(client_names));

    // allocate a server socket
    int server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
        error(1, errno, "chat_server: socket");

    // set SO_REUSEADDR so we can quickly bind to this address again if necessary
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 }, sizeof(int)) == -1)
        error(1, errno, "chat_server: setsockopt");

    // bind to all interfaces on given port
    struct sockaddr_in server_addr = 
    {
        .sin_family = AF_INET,    
        .sin_addr.s_addr = htonl(INADDR_ANY), 
        .sin_port = htons(port),
    };

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        error(1, errno, "chat_server: bind");

    // listen on port
    if (listen(server_sock, 16) == -1)
        error(1, errno, "chat_server: listen");

    // set up fd sets for select call
    fd_set ready_fd_set, active_fd_set, clients_fd_set;
    FD_ZERO(&ready_fd_set);
    FD_ZERO(&active_fd_set);
    FD_ZERO(&clients_fd_set);

    // add server socket and stdin to 
    FD_SET(0, &active_fd_set);
    FD_SET(server_sock, &active_fd_set);

    char message[MAX_MESSAGE_SIZE];
    memset(message, 0, sizeof(message));

    set_username();
    printf("\nHello %s!\n", username);
    printf("For a list of commands, please type /help\n\n");

    client_names[0] = username;
    prompt();

    while (1) {
        // set ready set to active set, since select call modifies in place
        ready_fd_set = active_fd_set;
        if ( select(FD_SETSIZE, &ready_fd_set, NULL, NULL, NULL) == -1 )
            error(1, errno, "chat_server: select");

        int i;
        for (i = 0; i < FD_SETSIZE; i++) {

            // if not a ready fd
            if ( !FD_ISSET(i, &ready_fd_set) )
                continue;

            // user input
            else if (i == 0) {
                errno = 0;
                fgets(message, sizeof(message)-1, stdin);
                if (errno == 0) {
                    server_command(client_names, &clients_fd_set, &active_fd_set, message);
                    prompt();
                }
                continue;
            }

            // new client connecting
            else if (i == server_sock) {
                errno = 0;
                int client_fd = accept(server_sock, (struct sockaddr*)NULL, NULL);
                
                // if error
                if (errno != 0) {
                    perror("chat_server: accept");
                    continue;
                }
                
                // if there was not an error receiving client, place on active list
                if( !receive_client(client_fd, client_names) ) {
                    FD_SET(client_fd, &active_fd_set);
                    FD_SET(client_fd, &clients_fd_set);
                }

                // otherwise disconnect them
                else {
                    close(client_fd);
                    continue;
                }

                // broadcast message saying they joined
                broadcast(client_fd, client_names, clients_fd_set, "%s has joined the chat.\r\n");
                prompt();
                continue;
            }

            // If here, client sent a message
            // check if client disconnected
            else if ( handle_client(i, client_names, clients_fd_set) ) {
                
                // remove from fd_sets and broadcast their departure
                FD_CLR(i, &active_fd_set);
                FD_CLR(i, &clients_fd_set);
                broadcast(i, client_names, clients_fd_set, "%s has left the chat.\r\n");
                
                // close socket and clear name
                close(i);
                client_names[i][0] = 0;

                prompt();
            }
        }
    }

    return;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("USAGE: %s <port>\n", argv[0]);
        return 1;
    }

    size_t port = strtoul(argv[1], NULL, 0);

    if (port > 0x10000 || port == 0) {
        fprintf(stderr, "Invalid port number.\n");
        exit(1);
    }

    printf("\n"
           "                          VVVVVV\n"
           "                         VVVVVV\n"
           "                        VVVVVV\n"
           "                       VVVVVV\n"
           " VVVVVV               VVVVVV\n"
           "  VVVVVV             VVVVVV\n"
           "   VVVVVV           VVVVVV\n"
           "    VVVVVV         VVVVVV CCCCCCC HHH    HHH     AA  TTTTTTTTTTT\n"
           "     VVVVVV       VVVVVV CCCCCCCC HHH    HHH    AAAA TTTTTTTTTTT\n"
           "      VVVVVV     VVVVVV CCCC      HHH    HHH   AA  AA    TTT\n"
           "       VVVVVV   VVVVVV  CCC       HHHHHHHHHH  AAA  AAA   TTT\n"
           "        VVVVVV VVVVVV   CCC       HHHHHHHHHH AAAAAAAAAA  TTT\n"
           "         VVVVVVVVVVV    CCC       HHH    HHH AAA    AAA  TTT\n"
           "          VVVVVVVVV     CCCCCCCCC HHH    HHH AAA    AAA  TTT\n"
           "           VVVVVVV       CCCCCCCC HHH    HHH AAA    AAA  TTT\n\n");
    chat_server(port);
    return 0;
}
