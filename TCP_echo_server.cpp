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
    // step 2: bind() associate socket with our IP and Port. Just run man bind, duh
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
        exit 1;
    }
    std::cout<<"Server is listening for incoming connections...\n";
}