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
void client_help() {
    printf("\nVChat v1.0 Help\n"
           "\t/help - Prints this help message\n"
           "\t/quit - Exits the client\n\n");
    return;
}

// Interprets a client command. If not preceded by a '/', treats
// it as a broadcast
int client_command(char *message, int sock) {
    // strip newline character
    message[strlen(message)-1] = 0;

    // if it is a command:
    if (message[0] == '/') {
        if (strcmp(message+1, "quit") == 0) {
            send_all(sock, "GOODBYE");
            printf("\33[2K\rBye...\n");
            exit(0);
        }

        if (strcmp(message+1, "help") == 0) {
            client_help();
            return 0;
        }
    }

    // otherwise broadcast
    else {

        // if it was just a newline, treat it as an accident and ignore
        if (message[0] == 0)
            return 1;

        char buffer[MAX_MESSAGE_SIZE];
        memset(buffer, 0, sizeof(buffer));

        snprintf(buffer, sizeof(buffer), "BROADCAST %s", message);
        send_all(sock, buffer);
        return 0;
    }

    return 0;
}

// Handles responses from the server
int handle_server(int server_fd) {
    char message[MAX_SEGMENT_SIZE];
    memset(message, 0, sizeof(message));

    // if no line ending
    if (recv_all(server_fd, message, sizeof(message))) {
        printf("\33[2K\r");
        fprintf(stdout, "ERROR - Malformed message from server!\n");
        return 1;
    }

    // If someone broadcasted a message
    if (strstr(message, "BROADCAST ") == message) {
        int len = strlen(message);
        message[len-2] = 0;
        printf("\33[2K\r");
        printf("%s\n", message + 10);
        fflush(stdout);
        return 0;
    }

    // server sent disconnect
    else if (strstr(message, "GOODBYE") == message) {
        fprintf(stderr, "\33[2K\rYou have been disconnected from the chat!\n");
        exit(1);
    }

    // Malformed message
    else {
        printf("\33[2K\r");
        fprintf(stderr, "ERROR - Malformed message from server! %s\n", message);
        return 0;
    }
}

// The main looping body. Connects to a server and interprets user input.
void chat_client(char *hostname, int port) {
    char message[MAX_MESSAGE_SIZE];
    memset(message, 0, sizeof(message));

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error(1, errno, "chat_client: socket");

    struct sockaddr_in server_addr = 
    {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };

    if (inet_aton(hostname, (struct in_addr*)&(server_addr.sin_addr.s_addr)) == -1)
        error(1, errno, "chat_client: inet_aton");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        error(1, errno, "chat_client: connect");

    // Request a username. If chosen name is already taken, prompt for another one.
    do {
        set_username();
        snprintf(message, sizeof(message), "HELLO I'M %s", username);
        send_all(sock, message);
        recv_all(sock, message, sizeof(message));

        if (strcmp(message, "IN USE\r\n") == 0)
            printf("Sorry that username has been taken!\n");

        else if (strcmp(message, "WELCOME!\r\n") != 0) {
            fprintf(stderr, "Error - Too many attempts!\n");
            exit(1);
        }
    }
    while (strcmp(message, "WELCOME!\r\n") != 0);

    printf("\nHello %s!\n", username);
    printf("For a list of commands, please type /help\n\n");

    // set up fd sets for select call
    fd_set ready_fd_set, active_fd_set;
    FD_ZERO(&ready_fd_set);
    FD_ZERO(&active_fd_set);

    // add server socket and stdin to 
    FD_SET(0, &active_fd_set);
    FD_SET(sock, &active_fd_set);

    printf("Welcome to the chat!\n");
    prompt();

    while (1) {
        // set ready set to active set, since select call modifies in place
        ready_fd_set = active_fd_set;
        if ( select(FD_SETSIZE, &ready_fd_set, NULL, NULL, NULL) == -1 )
            error(1, errno, "chat_client: select");

        // if user input
        if ( FD_ISSET(0, &ready_fd_set) ) {
            errno = 0;
            fgets(message, sizeof(message)-1, stdin);
            if (errno == 0) {
                client_command(message, sock);
                prompt();
            }
        }

        // server sent a message
        else if (FD_ISSET(sock, &ready_fd_set)) {
            // error
            if (handle_server(sock))
                return;

            prompt();
        }
    }

    return;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("USAGE: %s <hostname> <port>\n", argv[0]);
        return 1;
    }

    size_t port = strtoul(argv[2], NULL, 0);

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

    chat_client(argv[1], port);
    return 0;
}