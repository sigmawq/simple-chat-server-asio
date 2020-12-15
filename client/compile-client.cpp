#include <iostream>
#include "client.h"

using namespace boost::asio;

int main(int argc, char* argv[]) {
    std::cout << "---Simple chat client---" << std::endl;
    if (argc < 4){
        std::cout << "Usage: scs-client [address] [port] [username]" << std::endl;
        return 0;
    }
    std::string address {argv[1]};
    std::string port {argv[2]};
    std::string username(argv[3]);
    scs::launch(address, port, username);
    return 0;
}