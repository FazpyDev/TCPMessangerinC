#include "user.h"
#include "room.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Creates a clientSocket for the client
//Creates a user based on that clientSocket
//allocates memory in users array
//Adds user struct to that new allocated memory 
//Prompts user to enter password
void handleClientJoin(SOCKET listen_socket, User **users, size_t *usersLength, size_t *usersCapacity){
    //Create client socket

    printf("Client wants to connect \n");
    SOCKET clientSocket = INVALID_SOCKET;
    clientSocket = accept(listen_socket, NULL, NULL);
    if(clientSocket == INVALID_SOCKET){
        printf("ERROR: Creating clientSocket error \n");
        return;
    }
    printf("Client socket is valid \n");

    //If users is full
    //allocate new memory
    if((*usersLength) == (*usersCapacity)){
        User *temp = realloc((*users), sizeof(User) * ((*usersLength) + 1));
        if(temp == NULL){
            printf("ERROR: Re allocation user space error \n");
            return;
        }
        (*usersCapacity)++;
        (*users) = temp;
        printf("Users reallocation is valid \n");
    }

    //Add user to the new memory

    (*users)[(*usersLength)].clientSocket = clientSocket;
    (*users)[(*usersLength)].connectedRoomID = SIZE_MAX; // The placeholder val
    (*usersLength)++;

    //Prompts user to enter the room password.

    char message[100] = "[SYSTEM] Enter Room Password: ";
    send(clientSocket, message, strlen(message), 0);
}

//Shifts all sockets in the room's connectedClientSockets array
//so that the client socket that needs to be removed is last
//remove the size of the user struct's worth of memory at the end of the array
//Delete's room if there are no sockets
void removeClientfromRoom(User **users, size_t *usersLength,  Room **rooms, size_t *roomsLength, size_t *roomsCapacity, size_t userIndex){
    size_t connectedRoomIndex;
    bool found = findRoomIndexbyID((*users)[userIndex].connectedRoomID, (*rooms), (*roomsLength), &connectedRoomIndex);
    if(found == false){
        return;
    }

    Room *connectedRoom = &(*rooms)[connectedRoomIndex];
    size_t clientSocketIndex = SIZE_MAX;
    bool foundClientSocket = findClientSocket((*users)[userIndex].clientSocket, *connectedRoom, &clientSocketIndex);
    
    if(foundClientSocket == false){
        return;
    }

    removeClientSocket(clientSocketIndex, connectedRoom);

    char message[512] = "[SYSTEM] Other user has disconnected. They can join back (or a new user) using the same password. \n";
    sendSYSMessageinRoom((*connectedRoom), message, strlen(message));

    printf(
        "DEBUG: roomsLength = %zu, roomsCapacity = %zu\n",
        *roomsLength,
        *roomsCapacity
    );

    if(connectedRoom->numberOfConnectedSockets == 0){
        //delete room
        deleteRoom(rooms, connectedRoom, roomsLength, roomsCapacity, connectedRoomIndex);
    }

    printf(
        "DEBUG: roomsLength = %zu, roomsCapacity = %zu\n",
        *roomsLength,
        *roomsCapacity
    );

}

//Removes client from room (if inside one) using function "removeClientfromRoom"
//Removes client from users array
//by shifting the user to the very end of the array
//and lowering the usersLength value
void handleClientLeave(User **users, size_t *usersLength, size_t userIndex, Room **rooms, size_t *roomsLength, size_t *roomsCapacity){
        printf("MSG: Client wants to leave \n");

        //Explenation on this if statement is found earlier
        if((*users)[userIndex].connectedRoomID != SIZE_MAX){
            removeClientfromRoom(users, usersLength, rooms, roomsLength, roomsCapacity, userIndex);
        }

        closesocket((*users)[userIndex].clientSocket);
        (*users)[userIndex] = (*users)[(*usersLength) - 1];
        (*usersLength)--;
}

//Send a message to all connected Client (sockets) in a room
//loops through every socket in the rooms connectedClientSockets
//sends bytes inside "message"
void sendMessageinRoom(Room connectedRoom, char message[DEFAULT_BUFFERSIZE], int messageLength, User currentUser){
    for(size_t j = 0; j < connectedRoom.numberOfConnectedSockets; j++){
        SOCKET recieverSocket = connectedRoom.connectedClientSockets[j];
        if((recieverSocket) == INVALID_SOCKET){
            printf("ERROR: Reciever Socket error \n"); 
        }
        if(recieverSocket == (currentUser.clientSocket)){ // The sender will already see it in their CLI.
            continue;
        }
        printf("reciever socket is valid \n");
        int iSendResult = send((recieverSocket), message, messageLength, 0);
        if(iSendResult < 0){
            printf("ERROR: Sending error \n"); 
        }else{
            printf("SUCC: Sending message was successfull \n");
        }
        
    }
}

//Same as above but it also sends it to the sender.
void sendSYSMessageinRoom(Room connectedRoom, char message[DEFAULT_BUFFERSIZE], size_t messageLength){
    for(size_t j = 0; j < connectedRoom.numberOfConnectedSockets; j++){
        SOCKET recieverSocket = connectedRoom.connectedClientSockets[j];
        if((recieverSocket) == INVALID_SOCKET){
            printf("ERROR: Reciever Socket error \n"); 
            continue;
        }
        printf("reciever socket is valid \n");
        int iSendResult = send((recieverSocket), message, messageLength, 0);
        if(iSendResult < 0){
            printf("ERROR: Sending error \n"); 
        }else{
            printf("SUCC: Sending message was successfull \n");
        }
    }
}