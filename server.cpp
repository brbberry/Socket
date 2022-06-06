// Blake Berry
// 05/30/2022
// Homework 4
// This file is an implimentation of a server using TCP protcols for socket
// communication. Each service request generates a thread to address the client
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
using namespace std;

const int BUFFSIZE = 1500;
const int NUM_CONNECTIONS = 5;

// The paramter structure for the service thread
typedef struct serviceParam
{
    int clientSD_;
    char *dataBuf_;
    int numItters_;

} ServiceParam;

//-------------------------- buildAddress -------------------------------------
// Builds a socket address given a port number
// Preconditions : The port number is assumed to be valid
// Postconditions: Modifies the sockadd_in by refrence setting it up with the
//                 given port
void buildAddress(int port, sockaddr_in &acceptSocketAddress)
{
    bzero((char *)&acceptSocketAddress, sizeof(acceptSocketAddress));
    acceptSocketAddress.sin_family = AF_INET;
    acceptSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSocketAddress.sin_port = htons(port);
}

//------------------------- OpenSocketAndBind ---------------------------------
// Opens a socket and binds using the TCP protocol
// Preconditions : The socketaddress must be set up using buildaddress()
// Postconditions: Creates a server socket and binds the server to it
int openSocketAndBind(sockaddr_in &acceptSocketAddress)
{
    int serverSD = socket(AF_INET, SOCK_STREAM, 0);
    const int on = 1;
    setsockopt(serverSD, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));
    cout << "Socket #: " << serverSD << endl;

    int rc = bind(serverSD, (sockaddr *)&acceptSocketAddress, sizeof(acceptSocketAddress));
    if (rc < 0)
    {
        cerr << "Bind Failed" << endl;
    }
    return serverSD;
}

//-------------------------- readItters ---------------------------------------
// Gets the number of itterations for reading from the client
// Preconditions : The number of itterations is assumed to be valid
// Postconditions: Returns the integer version of the number of itterations
int readItters(int clienSD, char *databuf)
{
    int numRead = 0;
    uint32_t networkNumItters;
    while (numRead != sizeof(uint32_t))
    {
        numRead += read(clienSD, &networkNumItters, sizeof(uint32_t));
    }

    int numItters = ntohl(networkNumItters);
    return numItters;
}

//-------------------------- readFromClient -----------------------------------
// Reads from the client the number of itterations it specified
// Preconditions : a valid client socket must be established
// Postconditions: Returns the number of read calls
int readFromClient(int clientSD, char *dataBuf, int numItters)
{
    int numRead = 0;
    int numReadCalls = 0;
    int ittersCompleted = 0;
    while (ittersCompleted < numItters)
    {
        while (numRead != BUFFSIZE)
        {
            numRead += read(clientSD, dataBuf, (BUFFSIZE - numRead));
            numReadCalls++;
        }
        numRead = 0;
        ittersCompleted++;
    }
    return numReadCalls;
}

//------------------------ writeNumReadsToClient ------------------------------
// Writes the number of reads the server performed to the client
// Preconditions : a valid client socket must be established
// Postconditions: writes to the client the number of reads performed
void writeNumReadsToClient(int clientSD, int numReads)
{
    int numWritten = 0;
    uint32_t networkNumReads = htonl(numReads);
    while (numWritten < sizeof(uint32_t))
    {
        numWritten += write(clientSD, &networkNumReads, sizeof(uint32_t));
    }
    return;
}

//-------------------------- serverThread -------------------------------------
// Server Thread operation that reads from the client and then writes back to
// the client the number of read calls performed
// Preconditions : a connection between the server and client must be
//                 established
// Postconditions: reads and writes from the client, closing the socket after
//                 and terminates the thread
void *severThread(void *arg)
{
    ServiceParam *service = static_cast<ServiceParam *>(arg);

    int numReadCalls = readFromClient(service->clientSD_,
                                      service->dataBuf_,
                                      service->numItters_);
    writeNumReadsToClient(service->clientSD_, numReadCalls);
    close(service->clientSD_);
    pthread_exit(0);
}

//----------------------------------- main ------------------------------------
// Runs the server on a given port, establishes the connection and reads and
// writes to the client
// Preconditions : Assumes the data from the client is valid and that the
//                 size of messages is always BUFFSIZE
// Postconditions: completes server client read writes
int main(int argc, char *argv[])
{
    /*
     * Build address
     */
    if (argc != 2)
    {
        return -1;
    }
    int port = atoi(argv[1]);
    sockaddr_in acceptSocketAddress;
    buildAddress(port, acceptSocketAddress);

    /*
     *  Open socket and bind
     */
    int serverSD = openSocketAndBind(acceptSocketAddress);

    /*
     *  listen and accept
     */
    listen(serverSD, NUM_CONNECTIONS); // setting number of pending connections
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    int newSD = -1;
    // assumes valid number of itters -- checked by client
    while (true)
    {
        char databuf[BUFFSIZE];
        newSD = accept(serverSD, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if (newSD == -1)
        {
            cout << "Connection Error: " << newSD << endl;
        }
        else
        {
            int numItters = readItters(newSD, databuf);
            ServiceParam serverRequestParam;
            serverRequestParam.clientSD_ = newSD;
            serverRequestParam.dataBuf_ = databuf;
            serverRequestParam.numItters_ = numItters;
            pthread_t id;
            int offset1 = 1;
            pthread_create(&id, NULL, severThread, (void *)&serverRequestParam);
        }
    }

    return 0;
}
