# Reliable File Transfer Over UDP
## CIS 457 - F16 - Project4

###Overview
The purpose of this project was to implement reliability in file transfers over UDP, so that UDP can support out of order delivery and dropped packets, similar to TCP. This was done by defining a new packet structure, called Reliable UDP (RUDP) and implementing a sliding window. This project consists of a server and a client, where the client sends requests to the server for specific files, and the server responds by transferring the requested files. The behavior of the server and client can be characterized as follows:

####Server
- Receives a request from a client
- Sends the requested file to the client
- Listens for acknowledgements
- Closes the connection to the client

####Client
- Requests a file from the server
- Receives and acknowledges packets from the server
- Writes packets to file
- Closes the connection to the server

The server and client were implemented in C in server.c and client.c respectively. Both programs make use of additional functions defined in rudp_packet.h and rudp_packet.c, and the server uses functions defined in window.h and window.c. The syntax to run the server and client, respectively is:

  ./server [Port #] [Timeout (seconds) (optional)]
  
  ./client [Port #] [Server IPv4 address] [Path to file (optional)]


###Reliable UDP Packets
The RUDP packet is implemented in rudp_packet.h as a struct with an unsigned 32 bit sequence number, an unsigned 8 bit type, an unsigned 32 bit checksum, and a data segment. The 32 bit sequence number was chosen to easily accommodate out of order delivery, since the client writes the packet to file at the exact offset in the file specified by seq_num * RUDP_DATA (the size of the data segment of the packet). A 32 bit sequence number allows for very large files to be supported. The 16 bit checksum uses the same implementation as IPv4 or ICMP checksum, and the 8 bit type field was chosen as a matter of convenience. With only 5 possible flag values, 3 bits would have been sufficient.


###The Sliding Window
The sliding window is implemented in window.h as an array of pointers to dynamically allocated RUDP packets with a corresponding array of integers specifying the size of each packet. The window also includes two indices, a head and a tail. The head index refers to the first non-null element of the array, i.e. the first packet in the window. This is 0 when the window is full, but as packets are removed from the window, head is incremented, and thus keeps track of where the first packet is and how far the window can be advanced. The tail index refers to the next available index to insert a new packet into. This is equal to WINDOW_SIZE when the window is full.

##Server
###Receiving Client Requests
The server sets up a UDP socket to listen for a client connection on the port specified as the first command line argument. Once a client connection is open, the server reads packets from the client, waiting for one that is formatted as an RUDP packet with the type flag set as SYN. If the checksum of the SYN packet is good, the server attempts to open the file specified in the body of the SYN packet. The server then sends a SYN_ACK packet to the client to acknowledge that the file request was received, and the body of the SYN_ACK package specifies whether or not the file was successfully opened. The server stops and waits for an acknowledgement before continuing. If no acknowledgement is received after a certain amount of time (specified as a command line parameter or a default of 100 ms), the server resends the SYN_ACK packet.

###Sending the File
If the requested file is successfully opened, the server calls the send_file function. This function creates a new sliding window, creates a child thread to listen for acknowledgements, and then loops until the entire file has been sent and acknowledged. In each loop, the server advances the window, fills the window with data from the file, and then sends the window. At the end of each loop, the server sleeps for a specified time period to wait for acknowledgements.
    
###Listening for Acknowledgements
In a separate thread, the server listens for acknowledgements being sent from the client. When an acknowledgement is received, the server removes the corresponding packet from the sliding window. A mutex semaphore is used to allow both threads safe access to the window.

###Closing the Connection
Once the file has finished being sent, the server sends an RUDP packet with END_SEQ flag set. This notifies the client that the end of the file has been reached, and that the connection should be terminated. The server waits for a specified time for an acknowledgement, and if no acknowledgement is received, it resends the END_SEQ packet up to MAX_ATTEMPTS(5) times. If after MAX_ATTEMPTS tries to send the END_SEQ, no acknowledgement has been received, the server terminates the connection.

##Client
###Requesting a File
Upon establishing a connection to the server specified by the port and IP address command line arguments, the client sends a SYN packet to the server with the filename of the requested file in the body of the packet. The client waits until a SYN_ACK flag is received before continuing. If no SYN_ACK is received, the client resends the SYN packet.

###Receiving and Acknowledging Packets
Once the server has acknowledged the request and notified the client that the file was successfully opened, the client starts a loop to receiving packets. Upon receiving an RUDP packet, the client verifies its checksum, and if the checksum is valid, sends an acknowledgement to the server for that packet.

###Writing to File
If the checksum of a received packet is good, the client writes the data segment to the file a a particular offset specified by the the packet’s seq_num * RUDP_DATA (the size of the data portion of the packet). This allows for out of order delivery of packets.

###Closing the Connection
Once the client receives and END_SEQ packet, it sends an acknowledgement, closes the file, and exits the loop. It then performs an orderly shutdown of the connection to the server.
