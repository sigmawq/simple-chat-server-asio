//
// Created by swql on 12/5/20.
/*
 * //TODO: Add description
 */
//

#ifndef UNTITLED_MESSAGE_H
#define UNTITLED_MESSAGE_H

#include <chrono>
#include <string>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <boost/asio.hpp>
#include "defs.h"

using namespace boost::asio;

namespace scs {
    constexpr int READBUFF_SIZE = 1024;
    constexpr int USERNAME_SIZE = 32;

    enum HEADER_CODE : int32_t {
        USERNAME, INFO, CHAT_MESSAGE,
    };

    enum HEADER_SUBCODE : int32_t {
        SIMPLE_PING
    };

    // Message pattern is used as a base for other messages
    struct message_base {
        HEADER_CODE code;
        char message[READBUFF_SIZE] {0};
        void erase(){
            memset(this, 0, sizeof(message_base));
        }
    };

    // Used for client to declare it's username to server
    struct message_user {
        HEADER_CODE code = USERNAME;
        char username[32] {0};

        void set_message(const char *username, size_t size) {
            memcpy(this->username, username, size);
        }
    };

    // Used for client-server different information statement (primarily errors)
    struct message_info {
        HEADER_CODE code = INFO;
        HEADER_SUBCODE sub_code;
    };

    // Code = 2
    // Used for simple chat messaging
    struct message_chat {
        HEADER_CODE code = CHAT_MESSAGE;
        char username[USERNAME_SIZE] {0};
        char message_body[READBUFF_SIZE - sizeof(username) - sizeof(code)] {0};

        void set_username(const char *message, size_t num) {
            memcpy(username, message, num);
        }

        void set_message(const char *message, size_t num) {
            memcpy(message_body, message, num);
        }
    };

    template<typename T> void erase_content(T* ptr){
        memset(ptr, 0, sizeof(T));
    }

    void send_message_raw(
            boost::asio::ip::tcp::socket &socket,
            message_base* message,
            size_t from,
            size_t to) {
        int msg_size = to - from;
        boost::asio::const_buffer cbuff(message, msg_size);

        auto on_send = [&socket, &message, &msg_size, &to](
                const boost::system::error_code &error,
                std::size_t bytes_transferred
        ) {
            if (!error) {
                if (bytes_transferred < msg_size) {
                    std::cout << "  Message partially sent.. (" << bytes_transferred << '/'
                              << msg_size << std::endl;
                    send_message_raw(socket, message, bytes_transferred, to);
                }
                std::cout << "Message sent!" << std::endl;
            } else {
                std::cout << "Error while sending a message!" << std::endl;
            }
        };
        socket.async_send(cbuff, on_send);
    }

    void send_chat_message(ip::tcp::socket* socket,
                           const char* message, size_t size){
        message_chat m;
        m.set_message(message, size);
        send_message_raw(*socket, (message_base*)&m, 0, READBUFF_SIZE);
    }

        /*
         * Listen for incoming messages, when a message arrives function forwards it to the
         * assigned handler of type bouncing_action_fptr
         * // TODO: add different (?) handlers for different errors
         */
        void start_listen_to_socket_continuously(
                ip::tcp::socket &socket,
                message_base* buffer,
                bouncing_action_fptr handler,
                int from, int to
        ) {
            std::size_t buffer_size = to - from;
            boost::asio::mutable_buffer cbuff{(void *) buffer, buffer_size};
            socket.async_receive(cbuff, [=, &socket](const boost::system::error_code &err, std::size_t bytes_read) {
                if (!err) {
                    if (bytes_read < buffer_size) {
                        start_listen_to_socket_continuously(socket, buffer, handler, bytes_read, to);
                        return;
                    } else {
                        handler(&socket, buffer);
                        buffer->erase();
                        start_listen_to_socket_continuously(socket, buffer, handler, 0, to);
                    }
                }
                // TODO: Handle different errors and "errors"
            });
        }

        // Convert message_base to one of "derived" structures
        template<typename T>
        [[nodiscard]] inline T *base_to_derived(message_base &base) {
            T *converted = reinterpret_cast<T *>(&base);
            (*converted) = T();
            return converted;
        }

        // Make sure convenience structure's sizes are less than main structure size
        constexpr size_t mbs = sizeof(message_base);
        static_assert(
                sizeof(message_user) <= mbs
                && sizeof(message_chat) <= mbs
                && sizeof(message_info) <= mbs
        );
    };
#endif //UNTITLED_MESSAGE_H
