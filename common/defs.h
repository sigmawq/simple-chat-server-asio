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

    typedef void(*bouncing_action_fptr)(ip::tcp::socket*, scs::message_base* mb);
    typedef void(*user_action_fptr)(
            ip::tcp::socket*,
            std::vector<std::string>&);
}

#endif //SIMPLE_CHAT_SERVER_DEFS_H
