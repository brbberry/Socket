## Description
This project creates a multithreaded server that will service at most 5 clients concurrently. The client and the server implement TCP/IP to do point-to-point communication over a network. The client is an active open while the server serves as a passive open. The client will search for the server socket and the server will bind a socket to its ip given the port. The server will then listen and accept the client connection. The client will then send over the number of iterations and the server will generate a client thread. The thread reads from the client (conversely the client writes of the server). The server logs the number of reads and then sends that information back to the client. The service thread ends up terminating (closing the socket to the client) and then the client program terminates. An illustration is included below.

## Project Specifications
* The client will first send a message to the server containing the number of 
1500 byte data reads it must perform.  
* After the server reads the data it will send an acknowledgment message back which includes the number of socket read() calls performed. 
* Three types of writes from the client will be implemented
  - Atomic writes
  - One large write
  - Multiple small writes
