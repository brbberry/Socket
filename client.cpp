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
#include <chrono>
#include <math.h>
using namespace std;

const int BUFFSIZE = 1500;
enum writeType
{
    MULTIPLE = 1,
    ATOMIC_MULTIPLE = 2,
    ONE_WRITE = 3
};

// TODO better understand what this does and rename
int getRC(char *serverName, char *serverPort)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int clientSD = -1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6*/
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = 0;              /* Optional Options*/
    hints.ai_protocol = 0;           /* Allow any protocol*/
    int rc = getaddrinfo(serverName, serverPort, &hints, &result);
    return rc;
}

int alertServerOfReps(int clientSD, char *repetition)
{
    char val[BUFFSIZE]{0};
    strcpy(val, repetition);
    int numWritten = 0;

    while (numWritten < BUFFSIZE)
    {
        numWritten += write(clientSD, val, BUFFSIZE);
    }

    return numWritten;
}

int multipleWrites(int clientSD, char **databuf, int bufsize, int nbufs)
{
    int totalElements = 0;
    for (int i = 0; i < nbufs; i++)
    {
        int numElemsPerWrite = 0;
        
        while (numElemsPerWrite != bufsize)
        {
            numElemsPerWrite += write(clientSD, databuf[i], bufsize);
        }
        totalElements += numElemsPerWrite;
    }
    return totalElements;
}

int atomicMultipleWrites(int clientSD, char **databuf, int bufsize, int nbufs)
{
    struct iovec vector[nbufs];
    int totalElements = 0;
    for (int i = 0; i < nbufs; i++)
    {
        vector[i].iov_base = databuf[i];
        vector[i].iov_len = bufsize;
    }
    while(totalElements < BUFFSIZE) {
        totalElements += writev(clientSD, vector, nbufs);
    }

    return totalElements;
}

int singleWrite(int clientSD, char **databuf, int bufsize, int nbufs)
{
    int totalWritten = 0;

    while(totalWritten != bufsize * nbufs) 
    {
        totalWritten += write(clientSD, databuf, bufsize * nbufs);
    }

    return totalWritten;
}

int writeToServer(int clientSD, char **databuf, int bufsize, int nbufs, int type, int numItters)
{
    int numBytesWritten = 0;
    int ittersCompleted = 0;
    while(ittersCompleted < numItters) {
        ittersCompleted++;
        switch (type)
        {
        case MULTIPLE:
            numBytesWritten += multipleWrites(clientSD, databuf, bufsize, nbufs);
            break;
        case ATOMIC_MULTIPLE:
            numBytesWritten += atomicMultipleWrites(clientSD, databuf, bufsize, nbufs);
            break;
        case ONE_WRITE:
            numBytesWritten += singleWrite(clientSD, databuf, bufsize, nbufs);
            break;
        default:
            return -1;
        }
    }
    return numBytesWritten;
}

int main(int argc, char *argv[])
{

    /*
     *  Argument validation
     */
    if (argc != 7)
    {
        cerr << "Usage: " << argv[0] << "serverName" << endl;
        return -1;
    }

    // store the argv arguments for clairty
    char *serverName = argv[1];
    char *serverPort = argv[2];
    char *repetition = argv[3];
    char *nbufs = argv[4];
    char *bufsize = argv[5];
    char *type = argv[6];

    int numItters = atoi(repetition);
    int numBufs = atoi(nbufs);
    int numBufSize = atoi(bufsize);
    int writeType = atoi(type);

    // we need to be assuredof 1500
    if (numBufs * numBufSize != BUFFSIZE)
    {
        cerr << "Incorrect num byes " << numBufs * numBufSize
             << " != " << BUFFSIZE << endl;
        return -1;
    }

    char **dataBuf = new char *[numBufs];

    for (int i = 0; i < numBufs; i++)
    {
        dataBuf[i] = new char[numBufSize];
    }

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int clientSD = -1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6*/
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = 0;              /* Optional Options*/
    hints.ai_protocol = 0;           /* Allow any protocol*/
    int rc = getaddrinfo(serverName, serverPort, &hints, &result);

    if (rc != 0)
    {
        cerr << "ERROR: " << gai_strerror(rc) << endl;
        exit(EXIT_FAILURE);
    }

    /*
     * Iterate through addresses and connect
     */
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        clientSD = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (clientSD == -1)
        {
            continue;
        }
        /*
         * A socket has been successfully created
         */
        rc = connect(clientSD, rp->ai_addr, rp->ai_addrlen);
        if (rc < 0)
        {
            cerr << "Connection Failed" << endl;
            close(clientSD);
            return -1;
        }
        else // success
        {
            break;
        }

        if (rp == NULL)
        {
            cerr << "No valid address" << endl;
            exit(EXIT_FAILURE);
        }
        else
        {
            cout << "Client Socket: " << clientSD << endl;
        }
        freeaddrinfo(result);
    }

    /*
     *  Write and read data over network
     */
    
    for (int i = 0; i < numBufs; i++)
    {
        for(int j = 0; j < numBufSize; j++) {
            dataBuf[i][j] = 'z';
        }
    }
    

    int repsSent = alertServerOfReps(clientSD, repetition);
    if (repsSent <= 0)
    {
        cerr << "Unable to send number of reps to server" << endl;
    }
    // START THE CLOCK
    chrono::_V2::steady_clock::time_point start = chrono::steady_clock::now();
    int bytesWritten = writeToServer(clientSD, dataBuf, numBufSize, numBufs, writeType, numItters);
    char ACK[BUFFSIZE];
    int numBytesRead = 0;
    while (numBytesRead != BUFFSIZE)
    {
        numBytesRead += read(clientSD, ACK, BUFFSIZE);
    }
    // end clock
    chrono::_V2::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Number of Reads: " << ACK << endl;
    std::chrono::duration<double> time = end - start;
    double gb = numItters*BUFFSIZE/pow(10.0,6.0);
    double totalTime = time.count();
    double gbs = gb/totalTime;
    cout << "Test = " << type <<  " Total Time = " << totalTime << " μs," 
         << "#reads = " << ACK << ", throughput = " << gbs << " Gbps" << endl;

    for (int i = 0; i < numBufs; i++)
    {
        delete[] dataBuf[i];
        dataBuf[i] = nullptr;
    }
    delete[] dataBuf;

    close(clientSD);

    return 0;
}
