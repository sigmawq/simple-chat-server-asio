#define LOGGER_TO_COUT

#include <iostream>
#include "../common/scs-utility.h"
#include "server.h"

constexpr int BACKLOG_SIZE = 30;


using namespace boost::asio;

int main(int argc, char* argv[]) {
    std::cout << "---Simple chat server---" << std::endl;
    if (argc < 2){
        std::cout << "Usage: scs-client [port]" << std::endl;
        std::terminate();
    }
    std::string port = argv[1];
    scs::launch(port);

    return 0;
}
