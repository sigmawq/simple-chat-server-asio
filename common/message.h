//
// Created by swql on 12/5/20.
/*
 * Communication protocol and common (used by both server and client) actions are defined in this header
 * message_base is a general structure that can be converted to it's "derived" (no inheritance) child structures
 * any message_* child structure will always be less or equal than message_base base structure.
 * message_base structure is currently defined to be 1024 bytes long (READBUFF_SIZE)
 */
// TODO: Add description for child structures
//

#ifndef UNTITLED_MESSAGE_H
#define UNTITLED_MESSAGE_H

#include <chrono>
#include <string>
#include <list>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <boost/asio.hpp>
#include "defs.h"
#include "../logger/logger.h"

using namespace boost::asio;

namespace scs {

    enum HEADER_CODE : int32_t {
        USERNAME, INFO, CHAT_MESSAGE,
    };

    enum HEADER_SUBCODE : int32_t {
        SIMPLE_PING,
        _ERROR_SECTION_START,
            ERROR_WRITE_NOT_ALLOWED, ERROR_USERNAME_FAILED, _ERROR_SECTION_END
    };

    bool is_error(HEADER_SUBCODE subcode){
        if (subcode > _ERROR_SECTION_START && subcode < _ERROR_SECTION_END){
            return true;
        }
        return false;
    }

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

    template<typename T>// Convert message_base to one of "derived" structures
    [[nodiscard]] inline T *base_to_derived(message_base &base) {
        T *converted = reinterpret_cast<T *>(&base);
        (*converted) = T();
        return converted;
    }

    // Generate message_base and cast it to T
    template<typename T>
    inline std::shared_ptr<T> get_new_message(){
        auto new_base = std::make_shared<message_base>();
        auto casted = std::reinterpret_pointer_cast<T>(new_base);
        *casted = T();
        return casted;
    }

    template<typename T>
    void send_message_raw(
            boost::asio::ip::tcp::socket &socket,
            std::shared_ptr<T> message,
            size_t from,
            size_t to, logger& log) {
        int msg_size = to - from;
        boost::asio::const_buffer cbuff(message.get(), msg_size);
        log.push(
                "Sending message of type ", typeid(message.get()).name(),
                '(', "code: ", message->code , ')',
                " to ", socket.remote_endpoint().address().to_string());

        auto on_send = [&socket, &log, message, msg_size, to](
                const boost::system::error_code &error,
                std::size_t bytes_transferred
        ) {
            if (!error) {
                if (bytes_transferred < msg_size) {
                    log.push("  Message partially sent.. (", bytes_transferred, '/', msg_size);
                    send_message_raw(socket, message, bytes_transferred, to, log);
                }
                log.push("Message sent");
            }
            else {
                log.push("Error while sending a message: ", error.message());
            }
        };
        socket.async_send(cbuff, on_send);
    }

    void send_chat_message(ip::tcp::socket& socket, std::string& message, const std::string& username, logger& log){
        auto m = get_new_message<message_chat>();
        m->set_message(message.c_str(), message.size());
        m->set_username(username.c_str(), username.size());
        send_message_raw(socket, m, 0, READBUFF_SIZE, log);
    }

    void send_username(ip::tcp::socket& socket, std::string& username, logger& log){
        auto m = get_new_message<message_user>();
        m->set_message(username.c_str(), username.size());
        send_message_raw(socket, m, 0, READBUFF_SIZE, log);
    }

    void send_info_code(ip::tcp::socket& socket, HEADER_SUBCODE subcode, logger& log){
        auto m = get_new_message<message_info>();
        m->sub_code = subcode;
        send_message_raw(socket, m, 0, READBUFF_SIZE, log);
    }

        /*
         * Listen for incoming messages, when a message arrives function forwards it to the assigned
         * handler of type bouncing_action_fptr. When socket read results in EOF then handler of type
         * eof_handler is invoked. */
        void start_listen_to_socket_continuously(
                ip::tcp::socket* socket, message_base* buffer,
                bouncing_action_fptr handler, eof_handler eof_h,
                int from, int to, logger& log
        ) {
            std::size_t buffer_size = to - from;
            boost::asio::mutable_buffer cbuff{(void *) buffer, buffer_size};
            socket->async_receive(
                    cbuff,[socket, to, from, handler, eof_h, buffer, buffer_size, &log]
            (const boost::system::error_code &err, std::size_t bytes_read) {
                if (!err) {
                    if (bytes_read < buffer_size) {
                        start_listen_to_socket_continuously(socket, buffer, handler,
                                                            eof_h, bytes_read, to, log);
                        return;
                    } else {
                        log.push("Received a message of type: ", buffer->code);
                        handler(socket, buffer);
                        buffer->erase();
                        start_listen_to_socket_continuously(socket, buffer, handler,
                                                            eof_h, 0, to, log);
                    }
                }
                else if (err.value() == boost::asio::error::eof){
                    eof_h(socket);
                }
                else {
                    log.push("Error while receiving from socket: ", err.message());
                }
            });
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
