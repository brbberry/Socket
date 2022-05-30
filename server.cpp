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

typedef struct serviceParam
{
    int clientSD_;
    char *dataBuf_;
    int numItters_;

} ServiceParam;

int readItters(int clienSD, char *databuf)
{
    int numRead = 0;
    while (numRead != BUFFSIZE)
    {
        numRead += read(clienSD, databuf, BUFFSIZE);
    }

    int numItters = atoi(databuf);
    return numItters;
}

int readFromClient(int clientSD, char *dataBuf, int numItters)
{
    int numRead = 0;
    int numReadCalls = 0;
    // we will eventually want mult by num itters

    int actuallyRead = 0;
    int ittersCompleted = 0;
    while(ittersCompleted < numItters) {
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

void writeNumReadsToClient(int clientSD, int numReads)
{
    string num = to_string(numReads);
    char val[BUFFSIZE];
    const char *cStringNum = num.c_str();
    size_t len = strlen(cStringNum);
    strcpy(val, cStringNum);

    int numBytesRead = 0;
    //cout << "Server writing on socket " << clientSD << " numReads: " << numReads << endl;
    while (numBytesRead < BUFFSIZE)
    {
        numBytesRead += write(clientSD, cStringNum, BUFFSIZE);
    }
}

void *severThread(void *arg)
{
    ServiceParam *service = static_cast<ServiceParam *>(arg);

    int numReadCalls = readFromClient(service->clientSD_, service->dataBuf_, service->numItters_);
    writeNumReadsToClient(service->clientSD_, numReadCalls);
    close(service->clientSD_);
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    int serverPort;
    char *serverName;
    char databuf[BUFFSIZE];
    bzero(databuf, BUFFSIZE);

    /*
     * Build address
     */
    int port = 30556;
    sockaddr_in acceptSocketAddress;
    bzero((char *)&acceptSocketAddress, sizeof(acceptSocketAddress));
    acceptSocketAddress.sin_family = AF_INET;
    acceptSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSocketAddress.sin_port = htons(port);

    /*
     *  Open socket and bind
     */
    int serverSD = socket(AF_INET, SOCK_STREAM, 0);
    const int on = 1;
    setsockopt(serverSD, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));
    cout << "Socket #: " << serverSD << endl;

    int rc = bind(serverSD, (sockaddr *)&acceptSocketAddress, sizeof(acceptSocketAddress));
    if (rc < 0)
    {
        cerr << "Bind Failed" << endl;
    }

    /*
     *  listen and accept
     */
    listen(serverSD, NUM_CONNECTIONS); // setting number of pending connections
    sockaddr_in newSockAddr;
    socklen_t newSockAddrSize = sizeof(newSockAddr);
    int newSD = -1;
    
    while (true)
    {
        char databuf[BUFFSIZE];
        newSD = accept(serverSD, (sockaddr *)&newSockAddr, &newSockAddrSize);
        if(newSD == -1 ) {
            cout << "Connection Error: " << newSD << endl;
        } else {
            cout << "Accepted Socket #: " << newSD << endl;
            int numItters = readItters(newSD,databuf);
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
