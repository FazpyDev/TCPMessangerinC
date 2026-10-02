#ifndef ROOM_H
#define ROOM_H

#include <winsock2.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_ROOMS_SIZE 100
#define DEFAULT_BUFFERSIZE 512

typedef struct User User;

typedef struct Room{
    SOCKET *connectedClientSockets;
    size_t numberOfConnectedSockets;
    size_t roomID;
    char password[100];
} Room;



//Creates a empty room with a password assigned to it
//                            ^ the one the user has given it
void createRoom(Room *newRoom, size_t roomsCreated, char password[DEFAULT_BUFFERSIZE]);

//Adds a existing room struct into the rooms array
//by allocating new memory to it
void addRoom(Room newRoom, Room **rooms, size_t *roomsLength, size_t *roomsCreated, size_t *roomsCapacity);

//Deletes a room, replaces the room that needs to be deleted by the last element
//and then removing the last element
//e.g. Room1, room2, room3, room4, we want to delete room2
//Step 1. replace room with last room
//Room1, room4, room3, room4
//Step2. remove last element
//Room1, room4, room3
void deleteRoom(Room **rooms, Room *room, size_t *roomLength, size_t *roomCapacity, size_t roomIndex);

//Adds the user to a room and assigns users connectedRoomID to the roomsID
//does this by allocating new memory to the rooms connectedClientSockets array
void joinRoom(User *user, Room *room);

//loops through rooms to find a room with the same ID as given
//then returns the id of the room that it found
//if a room isnt found, the return value will be false
//and approporaite measures will be taken
bool findRoomIndexbyID(size_t id, Room *rooms, size_t roomsLength, size_t *roomIndex);

//Shifts all sockets in the room's connectedClientSockets array
//so that the client socket that needs to be removed is last
//remove the size of the user struct's worth of memory at the end of the array
//Delete's room if there are no sockets
void removeClientSocket(size_t clientSocketIndex, Room *connectedRoom);

//Looks through a room to find the index of a socket
//This is used when removing a client from a room, since the client's
//socket index is needed to perform the shifting.
bool findClientSocket(SOCKET socket, Room room, size_t *socketIndex);

#endif