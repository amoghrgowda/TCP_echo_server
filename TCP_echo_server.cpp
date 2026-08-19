#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// I have heavily documented this project as well, as usual. Why would anyone ever read the man pages? 
constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
constexpr int BACKLOG = 10; // max pending connections in queue

int main(){
    // step 1 in TCP server creation: socket(), which returns a positive fd on success otherwise -1.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        std::cerr << "[ERROR] socket creation failed for some reason!\n";
        return 1;
    }
    // step 2: bind() associate socket with our IP and Port. (JUST READ MAN PAGES BRO)
    // The kernel has no bind(fd, IP, port) format. we have to fill it out the 1980's C-way..
    // We have to fill out the struct manually. Apparently just how it works in POSIX
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr)); // zeroing out all fields, including padding sin_zero.
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htons(0); // pointing to 0.0.0.0 so that bind can accept connections arriving on any interface.
    // Also, why is sin_addr a whole struct when it could have just been a int? Type safety is one reason..
    //... But I think that's just coping - it's there for historical reasons mainly. Back when IP addressing was classful, and we needed byte-level access.
    
    if(bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0){
        std::cerr<<"[ERROR] bind failed on port: "<< PORT<<"\n";
        close(server_fd);
        return 1;
    }
    std::cout<<"Socket successfully bound to 0.0.0.0:"<<PORT<<"\tHurray!\n";

    // step 3: listening
    if(listen(server_fd, BACKLOG)<0){
        std::cerr <<"[ERROR] listen() failed\n";
        close(server_fd);
        return 1;
    }
    std::cout<<"Server is listening for incoming connections...\n";
    
    // for any incoming requests, the server kernel completes 3 way handshake and places it in 'accept queue'.
    
    // accept() extracts a client from the accept queue and provide a NEW fd.
    struct sockaddr_in client_addr;

    // this new fd helps to not lock-down the whole server for a single conversation.
    // Basically, by separating the listening role from communication/data-transfer role.
    socklen_t client_len = sizeof(client_addr);

    std::cout<<"[INFO] accept blocking...\n";
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if(client_fd<0){
        std::cerr<<"[ERROR] accept() failed..\n";
        close(server_fd);
        return 1;
    }

    //let's try and print the IP and port of the client.
    char client_ip[16]; // 16 bytes for IPv4
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, 16); // ntop just means network to presentation (human readable format)
    uint16_t client_port = ntohs(client_addr.sin_port);
    std::cout<<"[INFO] Client connected. IP: "<<client_ip
            <<":"<<client_port
            <<". Client FD="<<client_fd<<"\n";

    // Echo loop here (main logic)
    char buffer[BUFFER_SIZE];

    //We have to loop over recv() and send() so as to make sure all data is properly read. 
    // recv() returns as soon as a single chunk is read.
    // send() returns as soon as the kernel buffer is full. Returns how much data was read by kernel buffer.
    while (true) {
        // Clear buffer before each read
        std::memset(buffer, 0, sizeof(buffer));

        // recv() blocks until the client sends data (atleast 1 byte) or closes the connection
        ssize_t no_of_bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // 0 here consumes the read bytes. set it to other flags like MSG_PEEK if you want non-destructive read.
        
        if (no_of_bytes_received > 0) {
            // Case 1: Data was received
            std::cout << "[RECV] Received " << no_of_bytes_received << " with MSG: " << buffer;
            if (buffer[no_of_bytes_received - 1] != '\n') std::cout << "\n"; // if the last recieved byte isn't a \n, add a new line manually

            // Sending back the EXACT same msg back to the client
            ssize_t bytes_sent = send(client_fd, buffer, no_of_bytes_received, 0);
            // client-side testing of internal send buffer working: send(client_fd, "\n", sizeof("\n"),0);
            if (bytes_sent < 0) {
                std::cerr << "[ERROR] send() failed: " << std::strerror(errno) << "\n";
                break;
            }
            std::cout << "[SEND] Echoed back " << bytes_sent << " bytes.\n";

        } else if (no_of_bytes_received == 0) {
            // Case 2: Client sent TCP FIN (called a graceful disconnect)
            std::cout << "[INFO] Client disconnected gracefully (received EOF / FIN).\n";
            break;

        } else {
            // Case 3: Error occurred on the socket
            std::cerr << "[ERROR] recv() failed: " << std::strerror(errno) << "\n";
            break;
        }
    }

    std::cout << "[INFO] Closing sockets and exiting.\n";
    close(client_fd);  // Closes the client TCP connection (sends FIN)
    close(server_fd);  // Stops listening on port 8080

    return 0;
}