//
// Created by swql on 12/12/20.
/*
 * This header is here only to avoid cyclic dependencies
 */
//

#ifndef SIMPLE_CHAT_SERVER_DEFS_H
#define SIMPLE_CHAT_SERVER_DEFS_H
#include <boost/asio.hpp>

using namespace boost::asio;

namespace scs{
    struct message_base;
    constexpr int READBUFF_SIZE = 1024;
    constexpr int USERNAME_SIZE = 32;

    typedef void(*bouncing_action_fptr)(ip::tcp::socket*, scs::message_base* mb);
    typedef void(*user_action_fptr)(
            ip::tcp::socket*,
            std::vector<std::string>&);
    typedef void(*eof_handler)(ip::tcp::socket*);

    class connection_record{
    public:
        bool is_allowed_to_write = false;
        ip::tcp::endpoint endpoint;
        std::string username;
        ip::tcp::socket socket;
        std::vector<char> buffer;

        connection_record(io_context& ios) :
                socket {ios}, buffer {0}
        {
            buffer.resize(READBUFF_SIZE);
        }
    };
}

#endif //SIMPLE_CHAT_SERVER_DEFS_H
