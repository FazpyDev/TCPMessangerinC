//Imports

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// gets the other functions

#include "room.h"
#include "user.h"

//Declerations

#define Default_Port "27015"
#define DEFAULT_BUFFERSIZE 512
#define MAX_ROOM_SIZE 100

//Structures

//Free's up all the allocated memory
void cleanUp(struct addrinfo **result, User **users, size_t usersLength, Room **rooms, size_t roomsLength, SOCKET listenSocket);

int main(){

    //Dynamic arrays

    size_t roomsCreated = 0;
    size_t roomsLength = 0;
    size_t roomsCapacity = 0;
    Room *rooms = NULL;

    size_t usersLength = 0;
    size_t usersCapacity = 0;
    User *users = NULL;

    fd_set readfds;
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
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(listen_socket,result->ai_addr, (int)result->ai_addrlen);
    if(iResult == SOCKET_ERROR){
        printf("ERROR: Binding error \n");
        freeaddrinfo(result);
        WSACleanup();
        closesocket(listen_socket);
        return 1;
    }
    if(listen(listen_socket, SOMAXCONN) == SOCKET_ERROR){
        printf("ERROR: Listen error \n");
        freeaddrinfo(result);
        WSACleanup();
        closesocket(listen_socket);
        return 1;
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
            handleClientJoin(listen_socket, &users, &usersLength, &usersCapacity);
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
                    handleClientLeave(&users, &usersLength, i, &rooms, &roomsLength, &roomsCapacity);
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
                                printf("MSG: Found room \n");
                                char message[512] = "[SYSTEM] Room Found! You have Joined the room. \n";
                                sendSYSMessageinRoom(rooms[j], message, strlen(message));

                                char message2[512] = "[SYSTEM] A User has joined your room. \n";
                                sendMessageinRoom(rooms[j], message2, strlen(message2), *currentUser);
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
                    send(currentSocket, message, strlen(message), 0);
                    //create a room with the password

                    Room newRoom;
                    createRoom(&newRoom, roomsCreated, recvBuffer); // Create a blank room with the given password
                    addRoom(newRoom, &rooms, &roomsLength, &roomsCreated, &roomsCapacity); // Add the room to the rooms array
                    joinRoom(currentUser, &(rooms[roomsLength - 1])); // assign the room's creator to that room

                }
                
            }
        }

    }

    //Clean up
    //free's up all the allocated memory
    cleanUp(&result, &users, usersLength, &rooms, roomsLength, listen_socket);

    return 0;
}

//frees memory
void cleanUp(struct addrinfo **result, User **users, size_t usersLength, Room **rooms, size_t roomsLength, SOCKET listenSocket){
    freeaddrinfo((*result));
    for(size_t i = 0; i < usersLength; i++){
        closesocket((*users)[i].clientSocket);
    }
    free((*users));
    closesocket(listenSocket);
    for(size_t i = 0; i < roomsLength; i++){
        free((*rooms)[i].connectedClientSockets);
    }
    free((*rooms));

    WSACleanup();
}

//gcc server.c -o server.exe -lws2_32
///ncat 127.0.0.1 27015
