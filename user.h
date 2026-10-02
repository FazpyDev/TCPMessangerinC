#ifndef USER_H
#define USER_H

#include <winsock2.h>
#include <stddef.h>

typedef struct User{
    SOCKET clientSocket;
    size_t connectedRoomID;
} User;

#define DEFAULT_BUFFERSIZE 512

typedef struct Room Room;

//Creates a clientSocket for the client
//Creates a user based on that clientSocket
//allocates memory in users array
//Adds user struct to that new allocated memory 
//Prompts user to enter password
void handleClientJoin(SOCKET listen_socket, User **users, size_t *usersLength, size_t *usersCapacity);

//Removes client from room (if inside one) using function "removeClientfromRoom"
//Removes client from users array
//by moving the user to the very end of the array
//and lowering the usersLength value
void handleClientLeave(User **users, size_t *usersLength, size_t userIndex, Room **rooms, size_t *roomsLength, size_t *roomsCapacity);

//The actual function that does the removal
//of client's socket in the connectedClientSockets
//in a seperate function for readability
void removeClientfromRoom(User **users, size_t *usersLength, Room **rooms, size_t *roomsLength, size_t *roomsCapacity, size_t userIndex);

//Send a message to all connected Client (sockets) in a room
//loops through every socket in the rooms connectedClientSockets
//sends bytes inside "message"
void sendMessageinRoom(Room connectedRoom, char message[DEFAULT_BUFFERSIZE], int messageLength, User currentUser);

//Same as above but it also sends it to the sender.
void sendSYSMessageinRoom(Room connectedRoom, char message[DEFAULT_BUFFERSIZE], size_t messageLength);


#endif