// Blake Berry
// 05/30/2022
// Homework 4
// This file is an implimentation of a client using TCP protcols for socket
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
#include <chrono>
#include <math.h>
using namespace std;

const int BUFFSIZE = 1500;

// An enum type for the passed in write style
enum writeType
{
    MULTIPLE = 1,
    ATOMIC_MULTIPLE = 2,
    ONE_WRITE = 3
};

//----------------------- intializeClientSocket -------------------------------
// Initializes the client socket using TCP communication protocol
// Preconditions : Assumes the server name and server port are not null
// Postconditions: if the socket connection is successful the client socket
//                 descriptor is returned otherwise the clinet program is
//                 exited
int intializeClientSocket(char *serverName, char *serverPort)
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
    return clientSD;
}

//---------------------- validateClientArguments-------------------------------
// Ensures that the users input arguments are valid
// Preconditions : the arguments are valid integers
// Postconditions: exits the client program if invalid input is used
void validateClientArguments(int numItters, int numBufs, int numBufSize,
                             int writeType)
{
    // we need to be assuredof 1500
    if (numBufs * numBufSize != BUFFSIZE)
    {
        cerr << "Incorrect num byes " << numBufs * numBufSize
             << " != " << BUFFSIZE << endl;
        exit(EXIT_FAILURE);
    }

    if (numItters <= 0)
    {
        cerr << "Incorrect number of Itters <= 0" << numItters << endl;
        exit(EXIT_FAILURE);
    }

    if (writeType != MULTIPLE && writeType != ATOMIC_MULTIPLE &&
        writeType != ONE_WRITE)
    {
        cerr << "Incorrect write type" << writeType << endl;
        exit(EXIT_FAILURE);
    }
}

//-------------------------- intializeMessage ---------------------------------
// Initalizes an arbitrary message that will be sent from the client to the
// server
// Preconditions : the values for numBufs and bufSize have been validated
// Postconditions: returns a matrix message
char **intializeMessage(int numBufs, int bufSize)
{
    char **message = new char *[numBufs];

    for (int i = 0; i < numBufs; i++)
    {
        message[i] = new char[bufSize];
        for (int j = 0; j < bufSize; j++)
        {
            message[i][j] = 'z';
        }
    }
    return message;
}

//-------------------------- freeMessage --------------------------------------
// Frees the message created to the sent to the server
// Preconditions : the message has been created and numBufs has been validated
// Postconditions: frees the memory of the message back to the OS and removes
//                 any dangling pointers
void freeMessage(char **message, int numBufs)
{
    for (int i = 0; i < numBufs; i++)
    {
        delete[] message[i];
        message[i] = nullptr;
    }
    delete[] message;
    message = nullptr;
}

//-------------------------- getServerReads -----------------------------------
// Reads from the server the number of system read calls performed
// Preconditions : Assumes a valid server connection has been established
// Postconditions: returns the number of reads performed by the server
int getSeverReads(int clientSD)
{

    int numRead = 0;
    uint32_t serverNumReads;
    while (numRead != sizeof(uint32_t))
    {
        numRead += read(clientSD, &serverNumReads, sizeof(uint32_t));
    }

    int numReads = ntohl(serverNumReads);
    return numReads;
}

//----------------------- AlertServerOfReps -----------------------------------
// Writes to server the number of read itterations that need to be performed
// Preconditions : Assumes a valid server connection has been established
//                 Assumes that the number of itters is validated
// Postconditions: returns the number bytes written to the server
int alertServerOfReps(int clientSD, int numItters)
{

    int numWritten = 0;
    uint32_t clientNumItters = htonl(numItters);
    while (numWritten < sizeof(uint32_t))
    {
        numWritten += write(clientSD, &clientNumItters, sizeof(uint32_t));
    }

    return numWritten;
}

//----------------------- MultipleWrites ----------------------------------
// Writes to server the message in databuf -- where each write writes
// bufsize number of bytes nbuf times
// Preconditions : Assumes a valid server connection has been established
//                 Assumes that the bufsize and nbufs are valid
// Postconditions: returns the number bytes written to the server
int multipleWrites(int clientSD, char **databuf, int bufsize, int nbufs)
{
    int totalBytes = 0;
    for (int i = 0; i < nbufs; i++)
    {
        int numBytesPerWrite = 0;

        while (numBytesPerWrite != bufsize)
        {
            numBytesPerWrite += write(clientSD, databuf[i], bufsize);
        }
        totalBytes += numBytesPerWrite;
    }
    return totalBytes;
}

//----------------------- atomicMultipleWrites --------------------------------
// Writes to server the message in databuf -- where each write writes
// bufsize number of bytes nbuf times. Each write is atomic
// Preconditions : Assumes a valid server connection has been established
//                 Assumes that the bufsize and nbufs are valid
// Postconditions: returns the number bytes written to the server
int atomicMultipleWrites(int clientSD, char **databuf, int bufsize, int nbufs)
{
    struct iovec vector[nbufs];
    int totalBytes = 0;
    for (int i = 0; i < nbufs; i++)
    {
        vector[i].iov_base = databuf[i];
        vector[i].iov_len = bufsize;
    }
    while (totalBytes < BUFFSIZE)
    {
        totalBytes += writev(clientSD, vector, nbufs);
    }

    return totalBytes;
}

//----------------------- SingleWrite -----------------------------------------
// Writes to server the message in databuf -- only one write is performed
// Preconditions : Assumes a valid server connection has been established
//                 Assumes that the number of itters is validated
// Postconditions: returns the number bytes written to the server
int singleWrite(int clientSD, char **databuf, int bufsize, int nbufs)
{
    int totalWritten = 0;

    while (totalWritten != bufsize * nbufs)
    {
        totalWritten += write(clientSD, databuf, bufsize * nbufs);
    }

    return totalWritten;
}

//----------------------- WriteToServer ---------------------------------------
// Writes to the server a given the type of write and the number of itterations
// Preconditions : Assumes a valid server connection has been established
//                 Assumes that the number of itters is validated
// Postconditions: returns the number bytes written to the server
int writeToServer(int clientSD, char **databuf, int bufsize, int nbufs,
                  int type, int numItters)
{
    int numBytesWritten = 0;
    int ittersCompleted = 0;

    switch (type)
    {
    case MULTIPLE:
        while (ittersCompleted < numItters)
        {
            ittersCompleted++;
            numBytesWritten += multipleWrites(clientSD, databuf, bufsize,
                                              nbufs);
        }
        break;
    case ATOMIC_MULTIPLE:
        while (ittersCompleted < numItters)
        {
            ittersCompleted++;
            numBytesWritten += atomicMultipleWrites(clientSD, databuf,
                                                    bufsize, nbufs);
        }
        break;
    case ONE_WRITE:
        while (ittersCompleted < numItters)
        {
            ittersCompleted++;
            numBytesWritten += singleWrite(clientSD, databuf, bufsize, nbufs);
        }
        break;
    default:
        return -1;
    }

    return numBytesWritten;
}

//----------------------------------- main ------------------------------------
// Runs the client on a given port, establishes the connection and reads and
// writes to the server
// Preconditions : size of messages is always BUFFSIZE
// Postconditions: completes server client read writes
typedef chrono::_V2::steady_clock::time_point clockTime;
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

    validateClientArguments(numItters, numBufs, numBufSize, writeType);

    char **dataBuf = intializeMessage(numBufs, numBufSize);
    int clientSD = intializeClientSocket(serverName, serverPort);
    int repsSent = alertServerOfReps(clientSD, numItters);

    if (repsSent <= 0)
    {
        cerr << "Unable to send number of reps to server" << endl;
    }

    clockTime start = chrono::steady_clock::now();

    int bytesWritten = writeToServer(clientSD, dataBuf, numBufSize, numBufs,
                                     writeType, numItters);

    int numReads = getSeverReads(clientSD);

    clockTime end = chrono::steady_clock::now();

    cout << "Number of Reads: " << numReads << endl;

    chrono::duration<double> time = end - start;

    double gb = (8 * numItters * BUFFSIZE) / pow(10.0, 9.0);
    double totalTime = time.count();
    double gbs = gb / totalTime;

    cout << "Test = " << type << " Total Time = " << totalTime << " μs,"
         << "#reads = " << numReads << ", throughput = " << gbs << " Gbps"
         << endl;

    freeMessage(dataBuf, numBufs);
    close(clientSD);

    return 0;
}
