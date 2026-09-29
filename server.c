//Imports

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//Declerations

#define Default_Port "27015"
#define DEFAULT_BUFFERSIZE 512
#define MAX_ROOM_SIZE 100

//Structures

typedef struct{
    SOCKET *connectedClientSockets;
    size_t numberOfConnectedSockets;
    size_t roomID;
    char password[100];
} Room;

typedef struct{
    SOCKET clientSocket;
    size_t connectedRoomID;
} User;

// Functions {CODE BELOW MAIN FUNCTION} -- MORE DETAILED COMMENTS BELOW MAIN FUNCTION
// PLEASE READ: The actual code for the functions is below the main function
// This is because this allows the declerations of the functions to be
// in any order, and in general is good practice

// so if u want to see the code of the functions, scroll further down

//Creates a clientSocket for the client
//Creates a user based on that clientSocket
//allocates memory in users array
//Adds user struct to that new allocated memory 
//Prompts user to enter password
void handleClientJoin(SOCKET listen_socket, User **users, size_t *usersLength);

//Removes client from room (if inside one) using function "removeClientfromRoom"
//Removes client from users array
//by shifting the user to the very end of the array
//and lowering the usersLength value
void handleClientLeave(User **users, size_t *usersLength, size_t userIndex, Room **rooms, size_t *roomsLength);

//Looks through a room to find the index of a socket
//This is used when removing a client from a room, since the client's
//socket index is needed to perform the shifting.
bool findClientSocket(SOCKET socket, Room room, size_t *socketIndex);

//Shifts all sockets in the room's connectedClientSockets array
//so that the client socket that needs to be removed is last
//remove the size of the user struct's worth of memory at the end of the array
//Delete's room if there are no sockets
void removeClientSocket(size_t clientSocketIndex, Room *connectedRoom);

//Adds a existing room struct into the rooms array
//by allocating new memory to it
void addRoom(Room newRoom, Room **rooms, size_t *roomsLength, size_t *roomsCreated);

//Adds the user to a room and assigns users connectedRoomID to the roomsID
//does this by allocating new memory to the rooms connectedClientSockets array
void joinRoom(User *user, Room *room);

//Creates a empty room with a password assigned to it
//                            ^ the one the user has given it
void createRoom(Room *newRoom, size_t roomsCreated, char password[DEFAULT_BUFFERSIZE]);

//Deletes a room by performing the shifting
//and then reallocating the rooms array memory (removing the size of one Room)
void deleteRoom(Room **rooms, Room *room, size_t *roomLength, size_t roomIndex);

//loops through rooms to find a room with the same ID as given
//then returns the id of the room that it found
//if a room isnt found, the return value will be false
//and approporaite measures will be taken
bool findRoomIndexbyID(size_t id, Room *rooms, size_t roomsLength, size_t *roomIndex);

//The actual function that does the removal
//of client's socket in the connectedClientSockets
//in a seperate function for readability
void removeClientfromRoom(User **users, size_t *usersLength, Room **rooms, size_t *roomsLength, size_t userIndex);

//Send a message to all connected Client (sockets) in a room
//loops through every socket in the rooms connectedClientSockets
//sends bytes inside "message"
void sendMessageinRoom(Room connectedRoom, char message[DEFAULT_BUFFERSIZE], int messageLength, User currentUser);

//Same as above but it also sends it to the sender.
void sendSYSMessageinRoom(Room connectedRoom, char message[DEFAULT_BUFFERSIZE], size_t messageLength);

//Dynamic arrays

size_t roomsCreated = 0;
size_t roomsLength = 0;
Room *rooms = NULL;

size_t usersLength = 0;
User *users = NULL;

fd_set readfds;

int main(){
    //Set up winsock and listening Socket

    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, Default_Port, &hints, &result);

    SOCKET listen_socket = INVALID_SOCKET;
    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(listen_socket == INVALID_SOCKET){
        printf("ERROR: ListenSocket error \n");
    }

    iResult = bind(listen_socket,result->ai_addr, (int)result->ai_addrlen);
    if(iResult == SOCKET_ERROR){
        printf("ERROR: Binding error \n");
    }
    if(listen(listen_socket, SOMAXCONN) == SOCKET_ERROR){
        printf("ERROR: Listen error \n");
    }

    // Winsock and listen socket has been set up without error

    printf("Starting while Loop \n");

    //Starting clients -> server loop

    while(1){
        FD_ZERO(&readfds);

        printf("Starting FD_SETting \n");

        FD_SET(listen_socket, &readfds);

        //Add all client Sockets to fd_set to recieve packets

        for(size_t i = 0; i < usersLength; i++){
            printf("FD_SETTING user \n");
            FD_SET(users[i].clientSocket, &readfds);
        }

        printf("Finshed FD_SETTING sockets \n");

        select(0, &readfds, NULL, NULL, NULL);

        printf("Listening for client wanting to connect \n");

        if(FD_ISSET(listen_socket, &readfds)){
            //Client wants to connect
            //Add client to users dynamic array
            handleClientJoin(listen_socket, &users, &usersLength);
        }

        //Looping through clients to detect sent packets

        printf("Starting client loop \n");

        for(size_t i = 0; i < usersLength; i++){
            User *currentUser = &(users[i]);
            SOCKET currentSocket = users[i].clientSocket;

            
            //If client has sent a packet
            if(FD_ISSET(currentSocket, &readfds)){
                printf("Client has sent a message \n");
                char recvBuffer[DEFAULT_BUFFERSIZE];
                int iRecvResult;
                iRecvResult = recv(currentSocket, recvBuffer, DEFAULT_BUFFERSIZE - 1, 0);
                if(iRecvResult <= 0){
                    //0 bytes sent, Crash / close, therefore we want
                    //the client to leave

                    //remove client from any servers they were in (connectedClientSockets)
                    //clear memory client has taken (so in users and connectedClientSockets)
                    handleClientLeave(&users, &usersLength, i, &rooms, &roomsLength);
                    i--;
                    continue;
                }

                //add null terminator to message to not cause buffer overflow
                recvBuffer[iRecvResult] = '\0';
                printf("Client message recieved \n");
                
                //if user is in a room (SIZE_MAX is used as a placeholder value in order to detect wether a user is connected to a room)
                //If a user id is equal to SIZE_MAX, then they havent connected with a room.
                if((*currentUser).connectedRoomID != SIZE_MAX){
                    printf("User is already connected to a room. \n");
                    size_t connectedRoomIndex;
                    bool found = findRoomIndexbyID((*currentUser).connectedRoomID, rooms, roomsLength, &connectedRoomIndex);
                    if(found == true){
                        //Sends a message to all connected clients in that room
                        Room connectedRoom = rooms[connectedRoomIndex];
                        sendMessageinRoom(connectedRoom, recvBuffer, iRecvResult, *currentUser);
                    }
                    continue;
                }
                

                printf("Client dosen't have a room connected \n");
                bool foundRoom = false;
                printf("looking for room using password \n");
                
                //Client has entered a password to connect to a room
                //Look to see if a room has that password
                //if so, connect user to that room

                for(size_t j = 0; j < roomsLength; j++){
                    
                    if( (strcmp(recvBuffer, rooms[j].password)) == 0){
                        //found the room
                        foundRoom = true;
                        if(rooms[j].numberOfConnectedSockets < MAX_ROOM_SIZE){
                            // if the number of clients in that room is smaller 
                            // than the max allowed limit

                            //This function will allocate memory in the connectedClientSockets array inside of 
                            //The room, it will also assign the connectedRoomId of the user to the roomID of the Room.
                            joinRoom(currentUser, &rooms[j]);
                            break;
                        }
                        char message[DEFAULT_BUFFERSIZE] = "[SYSTEM] Room Found! However it is full. \n";
                        sendSYSMessageinRoom(rooms[j], message, DEFAULT_BUFFERSIZE);
                        
                        break;
                    }  
                }

                //If there isnt a room with that password
                //it will create a room with that empty password
                //and make the user join it.

                if(foundRoom == false){
                    printf("Room not found, creating a new room \n");
                    char message[DEFAULT_BUFFERSIZE] = "[SYSTEM] Room not found, new room created. \n";
                    send(currentSocket, message, DEFAULT_BUFFERSIZE, 0);
                    //create a room with the password

                    Room newRoom;
                    createRoom(&newRoom, roomsCreated, recvBuffer); // Create a blank room with the given password
                    addRoom(newRoom, &rooms, &roomsLength, &roomsCreated); // Add the room to the rooms array
                    joinRoom(currentUser, &(rooms[roomsLength])); // assign the room's creator to that room

                }
                
            }
        }

    }

    //Clean up
    //free's up all the allocated memory

    freeaddrinfo(result);
    WSACleanup();
    for(size_t i = 0; i < usersLength; i++){
        closesocket(users[i].clientSocket);
    }
    free(users);

    for(size_t i = 0; i < roomsLength; i++){
        for(size_t j = 0; j < rooms[i].numberOfConnectedSockets; j++){
            closesocket(rooms[i].connectedClientSockets[j]);
        }
        free(rooms[i].connectedClientSockets);
    }
    free(rooms);

    return 0;
}


//Creates a clientSocket for the client
//Creates a user based on that clientSocket
//allocates memory in users array
//Adds user struct to that new allocated memory 
//Prompts user to enter password
void handleClientJoin(SOCKET listen_socket, User **users, size_t *usersLength){
    //Create client socket

    printf("Client wants to connect \n");
    SOCKET clientSocket = INVALID_SOCKET;
    clientSocket = accept(listen_socket, NULL, NULL);
    if(clientSocket == INVALID_SOCKET){
        printf("ERROR: Creating clientSocket error \n");
        return;
    }
    printf("Client socket is valid \n");

    //Allocate new memory in users array

    User *temp = realloc((*users), sizeof(User) * ((*usersLength) + 1));
    if(temp == NULL){
        printf("ERROR: Re allocation user space error \n");
        return;
    }

    //Add user to the new memory

    printf("Users reallocation is valid \n");
    (*users) = temp;
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
void removeClientfromRoom(User **users, size_t *usersLength, Room **rooms, size_t *roomsLength, size_t userIndex){
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

    if(connectedRoom->numberOfConnectedSockets == 0){
        //delete room
        deleteRoom(rooms, connectedRoom, roomsLength, connectedRoomIndex);
    }
}

//Removes client from room (if inside one) using function "removeClientfromRoom"
//Removes client from users array
//by shifting the user to the very end of the array
//and lowering the usersLength value
void handleClientLeave(User **users, size_t *usersLength, size_t userIndex, Room **rooms, size_t *roomsLength){
        printf("MSG: Client wants to leave \n");

        //Explenation on this if statement is found earlier
        if((*users)[userIndex].connectedRoomID != SIZE_MAX){
            removeClientfromRoom(users, usersLength, rooms, roomsLength, userIndex);
        }

        closesocket((*users)[userIndex].clientSocket);
        (*users)[userIndex] = (*users)[(*usersLength) - 1];
        (*usersLength)--;
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
    for(size_t j = clientSocketIndex; j < connectedRoom->numberOfConnectedSockets - 1; j++){
        connectedRoom->connectedClientSockets[j] = connectedRoom->connectedClientSockets[j+1];
    }

    connectedRoom->numberOfConnectedSockets--;
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

//Adds a existing room struct into the rooms array
//by allocating new memory to it
void addRoom(Room newRoom, Room **rooms, size_t *roomsLength, size_t *roomsCreated){


    //Allocation
    Room *temp = realloc((*rooms), sizeof(Room) * ((*roomsLength) + 1));
    if(temp == NULL){
        printf("ERROR: Reallocating rooms error \n");
        return;
    }

    //Assignment
    printf("SUCC: Room reallocation successfull \n");
    (*rooms) = temp;
    (*rooms)[(*roomsLength)] = newRoom;
    (*roomsLength)++;
    (*roomsCreated)++;
    printf("MSG: User is already connected to a room. \n");
}

//Adds the user to a room and assigns users connectedRoomID to the roomsID
//does this by allocating new memory to the rooms connectedClientSockets array
void joinRoom(User *user, Room *room){
    printf("MSG: User is joining a room \n");
    (*user).connectedRoomID = (*room).roomID;

    //Allocation
    SOCKET *temp = realloc((*room).connectedClientSockets, sizeof(SOCKET) * ((*room).numberOfConnectedSockets + 1));
    if(temp == NULL){
        printf("ERROR: Joining room error");
        return;
    }

    //assignment
    printf("SUCC: User joining has been successfull.");
    (*room).connectedClientSockets = temp;
    (*room).connectedClientSockets[(*room).numberOfConnectedSockets] = (*user).clientSocket;
    (*room).numberOfConnectedSockets++;

    printf("MSG: Found room \n");
    char message[512] = "[SYSTEM] Room Found! You have Joined the room. \n";
    sendSYSMessageinRoom((*room), message, strlen(message));

    char message2[512] = "[SYSTEM] A User has joined your room. \n";
    sendSYSMessageinRoom((*room), message2, strlen(message2));
        
}

//Creates a empty room with a password assigned to it
//                            ^ the one the user has given it
void createRoom(Room *newRoom, size_t roomsCreated, char password[DEFAULT_BUFFERSIZE]){
    (*newRoom).roomID = (roomsCreated + 1);
    (*newRoom).numberOfConnectedSockets = 0;
    strcpy((*newRoom).password, password);
}
//Deletes a room by performing the shifting
//and then reallocating the rooms array memory (removing the size of one Room)
void deleteRoom(Room **rooms, Room *room, size_t *roomLength, size_t roomIndex){
    printf("Deleting room! \n");

    free((*room).connectedClientSockets);
    (*room).connectedClientSockets = NULL;

    for(size_t i = roomIndex; i < (*roomLength) - 1; i++){
        (*rooms)[i] = (*rooms)[i+1];
    }

    if((*roomLength) == 1){
        printf("le thing");
        free((*rooms));
        (*rooms) = NULL;
        (*roomLength)--;
        return;
    }

    Room *temp = realloc((*rooms), sizeof(Room) * ((*roomLength) - 1));
    if(temp == NULL){
        printf("ERROR: Delete room reallocation failed \n");
        return;
    }

    (*rooms) = temp;
    (*roomLength)--;
    printf("Deleting room successfull! \n");

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
    printf("Given ID: %d \n", id);
    return false;
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
            return;
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

//gcc server.c -o server.exe -lws2_32
///ncat 127.0.0.1 27015
