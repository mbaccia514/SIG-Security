// Simple definitions to be used by the simple_chat client and server

/*
// The VChat Protocol is defined as follows:
// - All messages must be terminated by a \r\n. Any message that does not include one
//   will be disgarded.
// 
// - When a user wishes to join a chatroom, they will send the following message:
//      HELLO I'M <username>\r\n
//   If that username has been taken, they will receive an "IN USE\r\n" reply. The
//   client repeats this process until they provide a valid username, in which they
//   they are accepted, or 10 tries, at which point they are dropped.
// 
// - Once a client has joined a chatroom, they may send "chats" using the following:
//      BROADCAST <message>\r\n
//   A client sends a BROADCAST to the server to send that message to all other clients.
//   A server sends a BROADCAST to each client to forward a chat from another user.
//
// - When a client wishes to disconnect, they send the following message:
//      GOODBYE\r\n
//   A server may also use this message to inform a client that they are being disconnected.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define USERNAME_SIZE 32
#define MAX_MESSAGE_SIZE 1024
#define MAX_SEGMENT_SIZE (MAX_MESSAGE_SIZE + 64)

char username[USERNAME_SIZE];

// Sets global username, removing the newline character.
void set_username() {
    do printf("Please enter a username (max 32): ");
    while(fgets(username, USERNAME_SIZE-1, stdin)[0] == '\n'); 

    // strip newline character
    username[strlen(username)-1] = 0;
    return;
}

// recv's from given fd into message until valid ending is found or until
// bytes received is greater than size. Anything after the ending is
// discarded.
int recv_all(size_t fd, char *message, size_t size) {
    char *buffer = message;
    int ending_flag = 0;

    int total = 0;
    while (total < size ) {
        int bytes_recv = recv(fd, buffer, size - total, 0);
        
        // error
        if (bytes_recv < 0)
            return 1;

        // end of send, no valid ending
        if (bytes_recv == 0)
            return 1;

        // place null terminator at end of this recv
        if ( (total + bytes_recv) == size)
            message[size-1] = 0;

        else
            message[total+bytes_recv] = 0;

        // if valid message end
        char *tmp = strstr(buffer, "\r\n");
        if (tmp != NULL) {
            message[size-1] = 0;

            if ( (((size_t)tmp + 2) - (size_t)message) < size )
                tmp[2] = 0;

            return 0;
        }

        total += bytes_recv;
        buffer += bytes_recv;
    }

    return 1;
}

// Sends message to fd, appending a \r\n to the message. 
int send_all(size_t fd, char *message) {
    char buffer[MAX_SEGMENT_SIZE];
    memset(buffer, 0, sizeof(buffer));

    strncpy(buffer, message, sizeof(buffer));
    strncat(buffer, "\r\n", sizeof(buffer));

    int to_be_sent = strlen(buffer);
    char *temp = buffer;

    while (to_be_sent) {
        int sent = send(fd, temp, to_be_sent, 0);
        
        // error
        if (sent == -1)
            return 1;

        temp += sent;
        to_be_sent -= sent;
    }

    return 0;
}

// Displays the prompt
void prompt() {
    printf("\33[2K\r");
    printf("%s: ", username);
    fflush(stdout);
    return;
}
