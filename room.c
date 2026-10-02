#include "room.h"
#include "user.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Creates a empty room with a password assigned to it
//                            ^ the one the user has given it
void createRoom(Room *newRoom, size_t roomsCreated, char password[DEFAULT_BUFFERSIZE]){
    (*newRoom).roomID = (roomsCreated + 1);
    (*newRoom).numberOfConnectedSockets = 0;
    (*newRoom).connectedClientSockets = NULL;
    strncpy(newRoom->password, password, sizeof(newRoom->password) - 1);
    newRoom->password[sizeof(newRoom->password) - 1] = '\0';
    
}

//Adds a existing room struct into the rooms array
//by allocating new memory to it
void addRoom(Room newRoom, Room **rooms, size_t *roomsLength, size_t *roomsCreated, size_t *roomsCapacity){

    //Allocation

    //if rooms is full
    //allocate new memory
    if((*roomsLength) == (*roomsCapacity)){
        Room *temp = realloc((*rooms), sizeof(Room) * ((*roomsLength) + 1));
        if(temp == NULL){
            printf("ERROR: Reallocating rooms error \n");
            return;
        }
        (*roomsCapacity)++;
        (*rooms) = temp;
        printf("SUCC: Room reallocation successfull \n");
    }

    //Assignment

    (*rooms)[(*roomsLength)] = newRoom;
    (*roomsLength)++;
    (*roomsCreated)++;
    printf("MSG: User is already connected to a room. \n");
}

//loops through rooms to find a room with the same ID as given
//then returns the id of the room that it found
//if a room isnt found, the return value will be false
//and approporaite measures will be taken
bool findRoomIndexbyID(size_t id, Room *rooms, size_t roomsLength, size_t *roomIndex){
    for(size_t i = 0; i < roomsLength; i++){
        if(rooms[i].roomID == id){
            (*roomIndex) = i;
            printf("Succ: Found room");
            return true;
        }
    }
    printf("ERROR: Roomindex by ID finding failed");
    printf("Given ID: %zu \n", id);
    return false;
}

//Looks through a room to find the index of a socket
//This is used when removing a client from a room, since the client's
//socket index is needed to perform the shifting.
bool findClientSocket(SOCKET socket, Room room, size_t *socketIndex){
        for(size_t j = 0; j < room.numberOfConnectedSockets; j++){
            if(room.connectedClientSockets[j] == (socket)){
                printf("SUCC: Found socket");
                (*socketIndex) = j;
                return true;
            }
        }
        printf("ERR: Not found socket");
        return false;
}

//The actual function that does the removal
//of client's socket in the connectedClientSockets
void removeClientSocket(size_t clientSocketIndex, Room *connectedRoom){
    connectedRoom->numberOfConnectedSockets--;
    connectedRoom->connectedClientSockets[clientSocketIndex] = connectedRoom->connectedClientSockets[connectedRoom->numberOfConnectedSockets];
    if(connectedRoom->numberOfConnectedSockets <= 0){
        free(connectedRoom->connectedClientSockets);
        connectedRoom->connectedClientSockets = NULL;
        return;
    }
    SOCKET *temp = realloc(connectedRoom->connectedClientSockets, sizeof(SOCKET) * (connectedRoom->numberOfConnectedSockets));
    if(temp == NULL){
        printf("ERROR: client leave reallocation failed \n");
        return;
    }
    printf("SUCC: client leaving has been successfull");
    connectedRoom->connectedClientSockets = temp;
}

//Adds the user to a room and assigns users connectedRoomID to the roomsID
//does this by allocating new memory to the rooms connectedClientSockets array
void joinRoom(User *user, Room *room){
    printf("MSG: User is joining a room \n");

    //Allocation
    SOCKET *temp = realloc((*room).connectedClientSockets, sizeof(SOCKET) * ((*room).numberOfConnectedSockets + 1));
    if(temp == NULL){
        printf("ERROR: Joining room error");
        return;
    }

    //assignment
    printf("SUCC: User joining has been successfull.");
    (*user).connectedRoomID = (*room).roomID;
    (*room).connectedClientSockets = temp;
    (*room).connectedClientSockets[(*room).numberOfConnectedSockets] = (*user).clientSocket;
    (*room).numberOfConnectedSockets++;


        
}

//Deletes a room, replaces the room that needs to be deleted by the last element
//and then removing the last element

//e.g. Room1, room2, room3, room4, we want to delete room2
//Step 1. replace room with last room
//Room1, room4, room3, room4
//Step2. remove last element
//Room1, room4, room3
void deleteRoom(Room **rooms, Room *room, size_t *roomLength, size_t *roomCapacity, size_t roomIndex){
    printf("Deleting room! \n");

    free((*room).connectedClientSockets);
    (*room).connectedClientSockets = NULL;

    if((*roomLength) == 1){
        free((*rooms));
        (*rooms) = NULL;
        (*roomLength)--;
        (*roomCapacity) = 0;
        return;
    }

    (*roomLength)--;
    (*rooms)[roomIndex] = (*rooms)[(*roomLength)];
    printf("Deleting room successfull! \n");
}